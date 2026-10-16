#include "save_data.h"

#include "bn_sram.h"

#include "castle.h"

namespace igb::save
{

namespace
{
    // "IGBA" in little-endian; bumping the version makes older saves convert (or read as empty).
    constexpr uint32_t magic = 0x41424749;
    constexpr uint16_t version = 2;
    constexpr uint16_t version_1 = 1;

    // SRAM layout: slot N's primary copy at N * slot_stride, its backup half a stride later.
    constexpr int slot_stride = 1024;
    constexpr int copy_stride = slot_stride / 2;

    // Version 1 (milestone 3), kept frozen so those saves load into the current format.
    struct record_v1
    {
        uint32_t magic;
        uint16_t version;
        uint16_t item_count;
        uint32_t checksum;

        uint16_t level;
        uint16_t unspent_points;
        uint32_t xp;
        uint32_t gold;
        uint16_t health;
        uint16_t attack;
        uint16_t shield;
        uint16_t magic_points;
        uint16_t ladder_step;
        uint8_t equipped[int(item_slot::COUNT)];
        uint8_t owned[roster::max_items];
        uint16_t item_xp[roster::max_items];
    };

    // Current version: fixed-size, packed; field sizes follow the progression spec's save table.
    struct record
    {
        uint32_t magic;
        uint16_t version;
        uint16_t item_count;           // Roster size when saved
        uint32_t checksum;             // Over every byte after this field

        uint16_t level;
        uint16_t unspent_points;
        uint32_t xp;
        uint32_t gold;
        uint16_t health;
        uint16_t attack;
        uint16_t shield;
        uint16_t magic_points;
        uint8_t equipped[int(item_slot::COUNT)];
        uint8_t owned[roster::max_items];
        uint16_t item_xp[roster::max_items];

        // Bloodline
        uint16_t keys;
        uint16_t bloodline;
        uint16_t cycle;

        // Mid-run state
        uint8_t in_run;
        uint8_t run_area;
        uint8_t run_cleared;
        int8_t run_duel_hotspot;
        uint16_t run_hp;
        uint32_t run_seed;
    };

    static_assert(sizeof(record) <= copy_stride, "A save copy no longer fits its SRAM space");
    static_assert(sizeof(record_v1) <= copy_stride);
    static_assert(slot_count * slot_stride <= bn::sram::size(), "Save slots don't fit in SRAM");

    constexpr int checksum_start = int(sizeof(uint32_t) * 3);  // After magic, version, item_count, checksum

    // FNV-1a over a record's data bytes (everything after the checksum field).
    template<typename Record>
    [[nodiscard]] uint32_t compute_checksum(const Record& data)
    {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&data);
        uint32_t hash = 2166136261u;

        for(int index = checksum_start; index < int(sizeof(Record)); ++index)
        {
            hash ^= bytes[index];
            hash *= 16777619u;
        }

        return hash;
    }

    [[nodiscard]] record to_record(const hero_profile& profile)
    {
        record data = {};
        data.magic = magic;
        data.version = version;
        data.item_count = uint16_t(roster::item_count);
        data.level = uint16_t(profile.level);
        data.unspent_points = uint16_t(profile.unspent_points);
        data.xp = uint32_t(profile.xp);
        data.gold = uint32_t(profile.gold);
        data.health = uint16_t(profile.health);
        data.attack = uint16_t(profile.attack);
        data.shield = uint16_t(profile.shield);
        data.magic_points = uint16_t(profile.magic);
        data.keys = uint16_t(profile.keys);
        data.bloodline = uint16_t(profile.bloodline);
        data.cycle = uint16_t(profile.cycle);
        data.in_run = profile.in_run ? 1 : 0;
        data.run_area = uint8_t(profile.run_area);
        data.run_cleared = uint8_t(profile.run_cleared);
        data.run_duel_hotspot = int8_t(profile.run_duel_hotspot);
        data.run_hp = uint16_t(profile.run_hp);
        data.run_seed = profile.run_seed;

        for(int slot = 0; slot < int(item_slot::COUNT); ++slot)
        {
            data.equipped[slot] = uint8_t(profile.equipped[slot]);
        }

        for(int index = 0; index < roster::max_items; ++index)
        {
            data.owned[index] = profile.owned[index] ? 1 : 0;
            int xp = profile.item_xp[index];
            data.item_xp[index] = uint16_t(xp < 0xFFFF ? xp : 0xFFFF);
        }

        data.checksum = compute_checksum(data);
        return data;
    }

    // Rejects records from another game, version or roster size, or with a bad checksum.
    template<typename Record>
    [[nodiscard]] bool valid(const Record& data, uint16_t expected_version)
    {
        return data.magic == magic && data.version == expected_version && data.item_count == roster::item_count &&
               data.checksum == compute_checksum(data);
    }

    // Fields every version has.
    template<typename Record>
    void read_common(const Record& data, hero_profile& profile)
    {
        profile = hero_profile();
        profile.level = data.level;
        profile.unspent_points = data.unspent_points;
        profile.xp = int(data.xp);
        profile.gold = int(data.gold);
        profile.health = data.health;
        profile.attack = data.attack;
        profile.shield = data.shield;
        profile.magic = data.magic_points;

        for(int index = 0; index < roster::max_items; ++index)
        {
            profile.owned[index] = data.owned[index] != 0;
            profile.item_xp[index] = data.item_xp[index];
        }

        // Equipped items must exist, sit in the right slot and be owned; otherwise use the starter.
        for(int slot = 0; slot < int(item_slot::COUNT); ++slot)
        {
            int index = data.equipped[slot];
            bool fits = index < roster::item_count && int(roster::items[index].slot) == slot && profile.owned[index];
            profile.equipped[slot] = fits ? index : roster::starter_item(item_slot(slot));
        }
    }

    void from_record(const record& data, hero_profile& profile)
    {
        read_common(data, profile);
        profile.keys = data.keys;
        profile.bloodline = data.bloodline > 0 ? data.bloodline : 1;
        profile.cycle = data.cycle;

        // A run in progress resumes only if its area and hotspot make sense.
        bool run_valid = data.in_run && data.run_area < castle::area_count;
        profile.in_run = run_valid;
        profile.run_area = run_valid ? data.run_area : castle::gate;
        profile.run_cleared = run_valid ? data.run_cleared : 0;
        profile.run_duel_hotspot = run_valid ? data.run_duel_hotspot : -1;
        profile.run_seed = data.run_seed;
        profile.run_hp = data.run_hp > 0 && data.run_hp <= profile.max_hp() ? data.run_hp : profile.max_hp();
    }

    // Reads one copy: the current version, or a version 1 save converted (no run, bloodline 1).
    [[nodiscard]] bool read_copy(int offset, hero_profile& profile)
    {
        record data;
        bn::sram::read_offset(data, offset);

        if(valid(data, version))
        {
            from_record(data, profile);
            return true;
        }

        record_v1 old_data;
        bn::sram::read_offset(old_data, offset);

        if(valid(old_data, version_1))
        {
            read_common(old_data, profile);
            return true;
        }

        return false;
    }

    [[nodiscard]] bool read_valid(int slot, hero_profile& profile)
    {
        int primary = slot * slot_stride;
        return read_copy(primary, profile) || read_copy(primary + copy_stride, profile);
    }
}

void write(int slot, const hero_profile& profile)
{
    record data = to_record(profile);
    int primary = slot * slot_stride;
    bn::sram::write_offset(data, primary);
    bn::sram::write_offset(data, primary + copy_stride);
}

bool read(int slot, hero_profile& profile)
{
    return read_valid(slot, profile);
}

slot_summary summary(int slot)
{
    slot_summary result;
    hero_profile profile;

    if(read_valid(slot, profile))
    {
        result.used = true;
        result.level = profile.level;
        result.gold = profile.gold;
        result.bloodline = profile.bloodline;
        result.in_run = profile.in_run;
    }

    return result;
}

void erase(int slot)
{
    record empty = {};
    int primary = slot * slot_stride;
    bn::sram::write_offset(empty, primary);
    bn::sram::write_offset(empty, primary + copy_stride);
}

}

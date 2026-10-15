#include "save_data.h"

#include "bn_sram.h"

namespace igb::save
{

namespace
{
    // "IGBA" in little-endian; bumping the version makes older saves read as empty.
    constexpr uint32_t magic = 0x41424749;
    constexpr uint16_t version = 1;

    // SRAM layout: slot N's primary copy at N * slot_stride, its backup half a stride later.
    constexpr int slot_stride = 1024;
    constexpr int copy_stride = slot_stride / 2;

    // Fixed-size, packed record; field sizes follow the progression spec's save table.
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
        uint16_t ladder_step;
        uint8_t equipped[int(item_slot::COUNT)];
        uint8_t owned[roster::max_items];
        uint16_t item_xp[roster::max_items];
    };

    static_assert(sizeof(record) <= copy_stride, "A save copy no longer fits its SRAM space");
    static_assert(slot_count * slot_stride <= bn::sram::size(), "Save slots don't fit in SRAM");

    constexpr int checksum_start = int(sizeof(uint32_t) * 3);  // After magic, version, item_count, checksum

    // FNV-1a over the record's data bytes.
    [[nodiscard]] uint32_t compute_checksum(const record& data)
    {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&data);
        uint32_t hash = 2166136261u;

        for(int index = checksum_start; index < int(sizeof(record)); ++index)
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
        data.ladder_step = uint16_t(profile.ladder_step);

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
    [[nodiscard]] bool valid(const record& data)
    {
        return data.magic == magic && data.version == version && data.item_count == roster::item_count &&
               data.checksum == compute_checksum(data);
    }

    void from_record(const record& data, hero_profile& profile)
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
        profile.ladder_step = data.ladder_step % tuning::fight_ladder_length;

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

    [[nodiscard]] bool read_copy(int offset, record& data)
    {
        bn::sram::read_offset(data, offset);
        return valid(data);
    }

    [[nodiscard]] bool read_valid(int slot, record& data)
    {
        int primary = slot * slot_stride;
        return read_copy(primary, data) || read_copy(primary + copy_stride, data);
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
    record data;

    if(! read_valid(slot, data))
    {
        return false;
    }

    from_record(data, profile);
    return true;
}

slot_summary summary(int slot)
{
    slot_summary result;
    record data;

    if(read_valid(slot, data))
    {
        result.used = true;
        result.level = data.level;
        result.gold = int(data.gold);
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

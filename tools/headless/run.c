// Headless mGBA runner for testing the ROM without a window: plays a script of held keys and
// saves screenshots. Build with tools/headless/build.sh (needs Homebrew's mgba), run with run.sh.
//
//   run <rom> <script> <output folder>
//
// Script lines:  wait N            run N frames with no keys
//                hold KEYS N       run N frames holding KEYS (e.g. A, B+LEFT, SELECT+R)
//                tap KEYS          hold for 2 frames, then release for 6
//                shot NAME         save <output folder>/NAME.ppm
//                # comment

#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba-util/vfs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* key_names[] = { "A", "B", "SELECT", "START", "RIGHT", "LEFT", "UP", "DOWN", "R", "L" };

static uint32_t parse_keys(char* text)
{
    uint32_t keys = 0;
    for(char* token = strtok(text, "+"); token; token = strtok(NULL, "+"))
    {
        for(int index = 0; index < 10; ++index)
        {
            if(strcmp(token, key_names[index]) == 0)
            {
                keys |= 1u << index;
            }
        }
    }
    return keys;
}

static void run_frames(struct mCore* core, uint32_t keys, int frames)
{
    core->setKeys(core, keys);
    for(int index = 0; index < frames; ++index)
    {
        core->runFrame(core);
    }
    core->setKeys(core, 0);
}

static void save_ppm(const char* path, const color_t* buffer, unsigned width, unsigned height)
{
    FILE* file = fopen(path, "wb");
    if(! file)
    {
        fprintf(stderr, "Can't write %s\n", path);
        return;
    }
    fprintf(file, "P6\n%u %u\n255\n", width, height);
    for(unsigned index = 0; index < width * height; ++index)
    {
        uint32_t color = buffer[index];
        unsigned char rgb[3] = { color & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff };
        fwrite(rgb, 1, 3, file);
    }
    fclose(file);
}

int main(int argc, char** argv)
{
    if(argc != 4)
    {
        fprintf(stderr, "usage: run <rom> <script> <output folder>\n");
        return 1;
    }

    struct mCore* core = mCoreFind(argv[1]);
    if(! core || ! core->init(core))
    {
        fprintf(stderr, "Can't open %s\n", argv[1]);
        return 1;
    }
    mCoreInitConfig(core, NULL);

    unsigned width, height;
    core->desiredVideoDimensions(core, &width, &height);
    color_t* buffer = calloc(width * height, sizeof(color_t));
    core->setVideoBuffer(core, buffer, width);

    if(! mCoreLoadFile(core, argv[1]))
    {
        fprintf(stderr, "Can't load %s\n", argv[1]);
        return 1;
    }
    core->reset(core);

    FILE* script = fopen(argv[2], "r");
    if(! script)
    {
        fprintf(stderr, "Can't open %s\n", argv[2]);
        return 1;
    }
    char line[256];
    int total = 0;
    while(fgets(line, sizeof(line), script))
    {
        char command[32] = "", arg1[128] = "";
        int count = 0;
        if(line[0] == '#' || sscanf(line, "%31s %127s %d", command, arg1, &count) < 1)
        {
            continue;
        }
        if(strcmp(command, "wait") == 0)
        {
            count = atoi(arg1);
            run_frames(core, 0, count);
        }
        else if(strcmp(command, "hold") == 0)
        {
            run_frames(core, parse_keys(arg1), count);
        }
        else if(strcmp(command, "tap") == 0)
        {
            run_frames(core, parse_keys(arg1), 2);
            run_frames(core, 0, 6);
            count = 8;
        }
        else if(strcmp(command, "shot") == 0)
        {
            char path[512];
            snprintf(path, sizeof(path), "%s/%s.ppm", argv[3], arg1);
            save_ppm(path, buffer, width, height);
            count = 0;
        }
        total += count;
    }
    fclose(script);
    printf("Ran %d frames\n", total);

    core->deinit(core);
    free(buffer);
    return 0;
}

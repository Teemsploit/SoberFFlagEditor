#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <json-c/json.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>

#define REL_PATH "/.var/app/org.vinegarhq.Sober/config/sober/config.json"

const char *ALLOWED_FLAGS[] = {
    "DFIntCSGLevelOfDetailSwitchingDistance",
    "DFIntCSGLevelOfDetailSwitchingDistanceL12",
    "DFIntCSGLevelOfDetailSwitchingDistanceL23",
    "DFIntCSGLevelOfDetailSwitchingDistanceL34",
    "FFlagHandleAltEnterFullscreenManually",
    "DFFlagTextureQualityOverrideEnabled",
    "DFIntTextureQualityOverride",
    "FIntDebugForceMSAASamples",
    "DFFlagDisableDPIScale",
    "FFlagDebugGraphicsPreferD3D11",
    "FFlagDebugSkyGray",
    "DFFlagDebugPauseVoxelizer",
    "DFIntDebugFRMQualityLevelOverride",
    "FIntFRMMaxGrassDistance",
    "FIntFRMMinGrassDistance",
    "FFlagDebugGraphicsPreferVulkan",
    "FFlagDebugGraphicsPreferOpenGL",
    "FIntGrassMovementReducedMotionFactor",

    NULL
};

static int is_flag_allowed(const char *key) {
    for (int i = 0; ALLOWED_FLAGS[i] != NULL; i++) {
        if (strcmp(key, ALLOWED_FLAGS[i]) == 0) return 1;
    }
    return 0;
}

static char *get_config_path(void) {
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        home = (pw ? pw->pw_dir : NULL);
    }
    if (!home) return NULL;

    size_t len = strlen(home) + strlen(REL_PATH) + 1;
    char *full = malloc(len);
    if (!full) return NULL;
    snprintf(full, len, "%s%s", home, REL_PATH);
    return full;
}

static void ensure_directories(const char *path) {
    char tmp[PATH_MAX];
    char *p = NULL;

    snprintf(tmp, sizeof(tmp), "%s", path);
    size_t len = strlen(tmp);
    if (len > 0 && tmp[len - 1] == '/') tmp[len - 1] = '\0';

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    }
    mkdir(tmp, S_IRWXU);
}

static json_object *load_config(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        json_object *root = json_object_new_object();
        json_object_object_add(root, "fflags", json_object_new_object());
        return root;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        json_object *root = json_object_new_object();
        json_object_object_add(root, "fflags", json_object_new_object());
        return root;
    }

    long sz = ftell(fp);
    if (sz < 0) {
        fclose(fp);
        json_object *root = json_object_new_object();
        json_object_object_add(root, "fflags", json_object_new_object());
        return root;
    }

    fseek(fp, 0, SEEK_SET);
    char *data = malloc((size_t)sz + 1);
    if (!data) {
        fclose(fp);
        return NULL;
    }
    fread(data, 1, (size_t)sz, fp);
    data[sz] = '\0';
    fclose(fp);

    json_object *root = json_tokener_parse(data);
    free(data);

    if (!root || !json_object_is_type(root, json_type_object)) {
        if (root) json_object_put(root);
        root = json_object_new_object();
        json_object_object_add(root, "fflags", json_object_new_object());
    }
    return root;
}

static int save_config(const char *path, json_object *root) {
    ensure_directories(path);
    const char *out = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY);
    FILE *fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "error: cannot write to %s: %s\n", path, strerror(errno));
        return -1;
    }
    fputs(out, fp);
    fclose(fp);
    return 0;
}

static int cmd_add_or_replace(const char *path, const char *key,
                               const char *val_str, int replace_only) {
    if (!is_flag_allowed(key)) {
        fprintf(stderr, "error: '%s' is not on the Roblox allowlist\n", key);
        return 1;
    }

    json_object *root = load_config(path);
    if (!root) {
        fprintf(stderr, "error: failed to load config\n");
        return 1;
    }

    json_object *ffs;
    if (!json_object_object_get_ex(root, "fflags", &ffs)) {
        ffs = json_object_new_object();
        json_object_object_add(root, "fflags", ffs);
    }

    json_object *existing = NULL;
    int exists = json_object_object_get_ex(ffs, key, &existing);

    int rc = 0;
    if (replace_only && !exists) {
        fprintf(stderr, "error: '%s' not set; use 'add' to create it\n", key);
        rc = 1;
    } else {
        json_object_object_add(ffs, key, json_object_new_string(val_str));
        rc = save_config(path, root);
    }

    json_object_put(root);
    return rc;
}

static void print_usage(const char *prog) {
    fprintf(stderr,
        "usage: %s <command> [args]\n"
        "\n"
        "commands:\n"
        "  add <flag> <value>      set a flag (creates or overwrites)\n"
        "  replace <flag> <value>  overwrite an existing flag (error if absent)\n"
        "  remove <flag>           delete a flag\n"
        "  list                    print the current config as JSON\n"
        "  clear                   remove all fflags\n"
        "  allowlist               print every flag on the Roblox allowlist\n"
        "  path                    print the config file path\n",
        prog);
}

int main(int argc, char **argv) {
    char *path = get_config_path();
    if (!path) {
        fprintf(stderr, "error: cannot determine home directory\n");
        return 1;
    }

    if (argc < 2) {
        print_usage(argv[0]);
        free(path);
        return 1;
    }

    int rc = 0;

    if (strcmp(argv[1], "add") == 0 && argc == 4) {
        rc = cmd_add_or_replace(path, argv[2], argv[3], 0);

    } else if (strcmp(argv[1], "replace") == 0 && argc == 4) {
        rc = cmd_add_or_replace(path, argv[2], argv[3], 1);

    } else if (strcmp(argv[1], "remove") == 0 && argc == 3) {
        json_object *root = load_config(path);
        if (!root) { rc = 1; goto done; }

        json_object *ffs;
        if (json_object_object_get_ex(root, "fflags", &ffs)) {
            json_object *existing = NULL;
            if (!json_object_object_get_ex(ffs, argv[2], &existing)) {
                fprintf(stderr, "error: flag '%s' is not set\n", argv[2]);
                rc = 1;
            } else {
                json_object_object_del(ffs, argv[2]);
                rc = save_config(path, root);
            }
        }
        json_object_put(root);

    } else if (strcmp(argv[1], "list") == 0) {
        json_object *root = load_config(path);
        if (!root) { rc = 1; goto done; }
        puts(json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY));
        json_object_put(root);

    } else if (strcmp(argv[1], "clear") == 0) {
        json_object *root = load_config(path);
        if (!root) { rc = 1; goto done; }
        json_object_object_add(root, "fflags", json_object_new_object());
        rc = save_config(path, root);
        json_object_put(root);

    } else if (strcmp(argv[1], "allowlist") == 0) {
        for (int i = 0; ALLOWED_FLAGS[i] != NULL; i++)
            puts(ALLOWED_FLAGS[i]);

    } else if (strcmp(argv[1], "path") == 0) {
        puts(path);

    } else {
        print_usage(argv[0]);
        rc = 1;
    }

done:
    free(path);
    return rc;
}

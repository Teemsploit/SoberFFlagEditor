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
    "FFlagRenderDebugCheckThreading2", "FFlagMovePrerenderV2", "FFlagMovePrerender",
    "FFlagDebugCheckRenderThreading", "FFlagFastGPULightCulling3", "FFlagDebugForceFSMCPULightCulling",
    "FIntTaskSchedulerThreadMin", "FIntRuntimeMaxNumOfThreads", "DFIntTaskSchedulerTargetFps",
    "DFFlagSkipHighResolutiontextureMipsOnLowMemoryDevices", "DFFlagSkipHighResolutiontextureMipsOnLowMemoryDevices2",
    "FFlagDebugGraphicsDisableDirect3D11", "DFIntCanHideGuiGroupId", "FFlagTaskSchedulerLimitTargetFpsTo2402",
    "DFIntRuntimeConcurrency",
    NULL // Null terminator for easy iteration
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
    snprintf(full, len, "%s%s", home, REL_PATH);
    return full;
}

static void ensure_directories(const char *path) {
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') tmp[len - 1] = 0;

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    }
}

static json_object *load_config(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        json_object *root = json_object_new_object();
        json_object_object_add(root, "fflags", json_object_new_object());
        return root;
    }

    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *data = malloc(sz + 1);
    fread(data, 1, sz, fp);
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

static void save_config(const char *path, json_object *root) {
    ensure_directories(path);
    const char *out = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY);
    FILE *fp = fopen(path, "w");
    if (fp) {
        fputs(out, fp);
        fclose(fp);
    }
}

static void cmd_add_or_replace(const char *path, const char *key, const char *val_str, int replace_only) {
    if (!is_flag_allowed(key)) {
        fprintf(stderr, "Error: Flag '%s' is not in the allowed list.\n", key);
        return;
    }

    json_object *root = load_config(path);
    json_object *ffs;
    if (!json_object_object_get_ex(root, "fflags", &ffs)) {
        ffs = json_object_new_object();
        json_object_object_add(root, "fflags", ffs);
    }

    json_object *existing = NULL;
    int exists = json_object_object_get_ex(ffs, key, &existing);

    if (replace_only && !exists) {
        printf("Key '%s' does not exist. Use 'add' instead.\n", key);
    } else {
        json_object_object_add(ffs, key, json_object_new_string(val_str));
        save_config(path, root);
        printf("Successfully set: %s = %s\n", key, val_str);
    }
    json_object_put(root);
}

int main(int argc, char **argv) {
    char *path = get_config_path();
    if (!path) return 1;

    if (argc < 2) {
        printf("Usage: %s [add|replace|remove|list|clear|path]\n", argv[0]);
        free(path);
        return 0;
    }

    if (strcmp(argv[1], "add") == 0 && argc == 4) {
        cmd_add_or_replace(path, argv[2], argv[3], 0);
    } 
    else if (strcmp(argv[1], "replace") == 0 && argc == 4) {
        cmd_add_or_replace(path, argv[2], argv[3], 1);
    }
    else if (strcmp(argv[1], "remove") == 0 && argc == 3) {
        json_object *root = load_config(path);
        json_object *ffs;
        if (json_object_object_get_ex(root, "fflags", &ffs)) {
            json_object_object_del(ffs, argv[2]);
            save_config(path, root);
        }
        json_object_put(root);
    }
    else if (strcmp(argv[1], "list") == 0) {
        json_object *root = load_config(path);
        printf("%s\n", json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY));
        json_object_put(root);
    }
    else if (strcmp(argv[1], "path") == 0) {
        printf("%s\n", path);
    }

    free(path);
    return 0;
}

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

static char *get_config_path(void) {
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        home = (pw ? pw->pw_dir : NULL);
    }
    size_t len = strlen(home) + strlen(REL_PATH) + 1;
    char *full = malloc(len);
    snprintf(full, len, "%s%s", home, REL_PATH);
    return full;
}

static void ensure_parent_dirs(const char *path) {
    char *copy = strdup(path);
    char *slash = strrchr(copy, '/');
    if (slash) {
        *slash = '\0';
        char accum[PATH_MAX] = {0};
        char *p = copy;
        while (*p == '/') p++;
        strcpy(accum, "/");
        for (char *q = p; *q; q++) {
            if (*q == '/') {
                size_t len_acc = strlen(accum);
                strncat(accum, p, (q - p));
                accum[len_acc + (q - p)] = '\0';
                if (mkdir(accum, 0755) != 0 && errno != EEXIST) {
                    free(copy);
                    exit(1);
                }
                strcat(accum, "/");
                p = q + 1;
            }
        }
        if (*p) {
            strcat(accum, p);
            if (mkdir(accum, 0755) != 0 && errno != EEXIST) {
                free(copy);
                exit(1);
            }
        }
    }
    free(copy);
}

static void ensure_config_exists(const char *path) {
    if (access(path, F_OK) == 0) return;
    ensure_parent_dirs(path);
    json_object *root = json_object_new_object();
    json_object *ff = json_object_new_object();
    json_object_object_add(root, "fflags", ff);
    const char *out = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY);
    FILE *fp = fopen(path, "w");
    if (!fp) {
        json_object_put(root);
        exit(1);
    }
    fputs(out, fp);
    fclose(fp);
    json_object_put(root);
}

static json_object *load_config(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) exit(1);
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *data = malloc(sz + 1);
    fread(data, 1, sz, fp);
    data[sz] = '\0';
    fclose(fp);

    json_object *root = json_tokener_parse(data);
    free(data);
    if (!root || json_object_get_type(root) != json_type_object) {
        if (root) json_object_put(root);
        root = json_object_new_object();
        json_object *ff = json_object_new_object();
        json_object_object_add(root, "fflags", ff);
    }
    return root;
}

static void save_config(const char *path, json_object *root) {
    const char *out = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY);
    FILE *fp = fopen(path, "w");
    if (!fp) exit(1);
    fputs(out, fp);
    fclose(fp);
}

static void print_help(const char *prog) {
    printf(
        "Usage:\n"
        "  %s add     <FLAG_KEY> <VALUE>\n"
        "  %s replace <FLAG_KEY> <VALUE>\n"
        "  %s remove  <FLAG_KEY>\n"
        "  %s list\n"
        "  %s clear\n"
        "  %s path\n"
        "  %s help\n",
        prog, prog, prog, prog, prog, prog, prog
    );
}

static void cmd_list(const char *config_path) {
    json_object *root = load_config(config_path);
    json_object *ff;
    if (json_object_object_get_ex(root, "fflags", &ff) &&
        json_object_get_type(ff) == json_type_object) {
        printf("%s\n", json_object_to_json_string_ext(ff, JSON_C_TO_STRING_PRETTY));
    } else {
        printf("{}\n");
    }
    json_object_put(root);
}

static void cmd_add(const char *config_path, const char *key, const char *value) {
    json_object *root = load_config(config_path);
    json_object *ff;
    if (!json_object_object_get_ex(root, "fflags", &ff) ||
        json_object_get_type(ff) != json_type_object) {
        ff = json_object_new_object();
        json_object_object_add(root, "fflags", ff);
    }
    json_object *existing = NULL;
    json_object_object_get_ex(ff, key, &existing);
    if (!existing) {
        json_object *val = json_object_new_string(value);
        json_object_object_add(ff, key, val);
    }
    save_config(config_path, root);
    json_object_put(root);
}

static void cmd_replace(const char *config_path, const char *key, const char *value) {
    json_object *root = load_config(config_path);
    json_object *ff;
    if (json_object_object_get_ex(root, "fflags", &ff) &&
        json_object_get_type(ff) == json_type_object) {
        json_object *existing = NULL;
        if (json_object_object_get_ex(ff, key, &existing)) {
            json_object *val = json_object_new_string(value);
            json_object_object_add(ff, key, val);
        }
    }
    save_config(config_path, root);
    json_object_put(root);
}

static void cmd_remove(const char *config_path, const char *key) {
    json_object *root = load_config(config_path);
    json_object *ff;
    if (json_object_object_get_ex(root, "fflags", &ff) &&
        json_object_get_type(ff) == json_type_object) {
        json_object_object_del(ff, key);
    }
    save_config(config_path, root);
    json_object_put(root);
}

static void cmd_clear(const char *config_path) {
    json_object *root = load_config(config_path);
    json_object *ff = json_object_new_object();
    json_object_object_add(root, "fflags", ff);
    save_config(config_path, root);
    json_object_put(root);
}

static void cmd_path(const char *config_path) {
    printf("%s\n", config_path);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    char *config_path = get_config_path();
    ensure_config_exists(config_path);

    if (strcmp(argv[1], "add") == 0 && argc == 4) {
        cmd_add(config_path, argv[2], argv[3]);
    }
    else if (strcmp(argv[1], "replace") == 0 && argc == 4) {
        cmd_replace(config_path, argv[2], argv[3]);
    }
    else if (strcmp(argv[1], "remove") == 0 && argc == 3) {
        cmd_remove(config_path, argv[2]);
    }
    else if (strcmp(argv[1], "list") == 0 && argc == 2) {
        cmd_list(config_path);
    }
    else if (strcmp(argv[1], "clear") == 0 && argc == 2) {
        cmd_clear(config_path);
    }
    else if (strcmp(argv[1], "path") == 0 && argc == 2) {
        cmd_path(config_path);
    }
    else if (strcmp(argv[1], "help") == 0 && argc == 2) {
        print_help(argv[0]);
    }
    else {
        print_help(argv[0]);
        free(config_path);
        return 1;
    }

    free(config_path);
    return 0;
}

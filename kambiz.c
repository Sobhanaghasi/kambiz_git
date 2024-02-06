#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#define MAX_PATH_LENGTH 1000
#define MAX_STRING_LENGTH 1000
#define MAX_FULL_COMMAND_LENGTH 500
#define MAX_WORD_COUNT 50
#define MAX_SHORT_COMMAND_LENGTH 50
#define MAX_INT_VALUE 1000000
#define MAX_FILE_SIZE 1000000
#define MAX_FORMAT_LENGTH 10

#define COLOR_RESET "\e[0m"
#define CYAN "\e[0;36m"
#define YELLOW "\e[0;33m"
#define RED "\e[0;31m"
#define GREEN "\e[0;32m"

char global_address[MAX_PATH_LENGTH] = "/Users/sobhan/Documents/Kambiz";
char local_address[MAX_PATH_LENGTH];
char user_name[MAX_STRING_LENGTH] = "default";
char user_email[MAX_STRING_LENGTH] = "default";
bool init_done = false;
bool on_head = false;
char current_branch_name[MAX_STRING_LENGTH] = "";
int current_id = 0;
int last_commit_id;

time_t string_to_time(char time_string[])
{
    struct tm temp;
    if (strptime(time_string, "%Y/%m/%d-%H:%M:%S", &temp) == NULL)
    {
        return -1;
    }
    return (mktime(&temp));
}

void split_by_chars(char source[], char destination1[], char destination2[], char splitters[])
{
    int char_position = strcspn(source, splitters);
    strncat(destination1, source, char_position);
    if (char_position != strlen(source))
    {
        strcat(destination2, source + char_position + 1);
    }
}

void get_value_by_attribute(FILE *file, char attribute[], char destination[], int value_count, int wanted_value, bool have_spaces)
{
    rewind(file);
    char line[MAX_STRING_LENGTH];
    long line_position = -1;
    while (fscanf(file, "%[^\n]s", line) != EOF)
    {
        char line_attribute[MAX_STRING_LENGTH] = "";
        sscanf(line, "%100s", line_attribute);
        if (strcmp(line_attribute, attribute) == 0)
        {
            line_position = ftell(file) - strlen(line);
            break;
        }
        fscanf(file, "\n");
    }

    if (line_position == -1)
    {
        rewind(file);
        return;
    }

    fseek(file, line_position + (wanted_value * 100), SEEK_SET);
    if (have_spaces)
    {
        fscanf(file, "%[^\n]s", destination);
    }
    else
    {
        fscanf(file, "%100s", destination);
    }
    rewind(file);
}

void change_value_by_attribute(FILE *file, char attribute[], char value[], int value_count, int wanted_value, bool have_spaces)
{
    char line[MAX_STRING_LENGTH];
    long line_position = -1;
    while (fscanf(file, "%[^\n]s", line) != EOF)
    {
        char line_attribute[MAX_STRING_LENGTH] = "";
        sscanf(line, "%100s", line_attribute);
        if (strcmp(line_attribute, attribute) == 0)
        {
            line_position = ftell(file) - strlen(line);
            break;
        }
        fscanf(file, "\n");
    }
    if (line_position == -1)
    {
        return;
    }

    fseek(file, line_position + (wanted_value * 100), SEEK_SET);
    if (have_spaces)
    {
        fprintf(file, "%s\n", value);
    }
    else
    {
        fprintf(file, "%-100s", value);
    }
}

int char_counter(char string[], char wanted_char)
{
    int count = 0;
    for (int i = 0; i < strlen(string); i++)
    {
        if (string[i] == wanted_char)
        {
            count++;
        }
    }
    return count;
}

void update_user_data()
{
    char global_user_address[MAX_PATH_LENGTH] = "/Users/sobhan/Documents/Kambiz/user.txt";
    char local_user_address[MAX_PATH_LENGTH] = ".kambiz/user.txt";

    FILE *global_user = fopen(global_user_address, "r");
    get_value_by_attribute(global_user, "name", user_name, 1, 1, false);
    get_value_by_attribute(global_user, "email", user_email, 1, 1, false);
    fclose(global_user);

    if ((strcmp(user_name, "default") == 0) && (init_done))
    {
        FILE *local_user = fopen(local_user_address, "r");
        get_value_by_attribute(local_user, "name", user_name, 1, 1, false);
        fclose(local_user);
    }

    if ((strcmp(user_email, "default") == 0) && (init_done))
    {
        FILE *local_user = fopen(local_user_address, "r");
        get_value_by_attribute(local_user, "email", user_email, 1, 1, false);
        fclose(local_user);
    }
}

void update_branch_name()
{
    FILE *current_branch_file = fopen(".kambiz/branches/current_branch.txt", "r");
    fscanf(current_branch_file, "%s", current_branch_name);
    fclose(current_branch_file);
}

void update_id()
{
    FILE *current_id_file = fopen(".kambiz/branches/current_id.txt", "r");
    fscanf(current_id_file, "%d", &current_id);
    fclose(current_id_file);
}

void update_last_commit_id()
{
    char last_commit_id_address[MAX_PATH_LENGTH] = ".kambiz/branches/last_commit_id.txt";
    FILE *last_commit_id_file = fopen(last_commit_id_address, "r");
    fscanf(last_commit_id_file, "%d", &last_commit_id);
    fclose(last_commit_id_file);
}

int find_branch_head_n_id(char branch_name[], int n)
{
    FILE *commit_log = fopen(".kambiz/branches/commit_log.txt", "r");
    int matches_found = 0;
    char line[601];
    while (fscanf(commit_log, "%[^\n]s", line) != EOF)
    {
        char log_branch_name[101];
        sscanf(line + 100, "%100s", log_branch_name);
        if (strcmp(log_branch_name, branch_name) == 0)
        {
            matches_found++;
            if (matches_found == n)
            {
                int id;
                sscanf(line, "%100d", &id);
                return id;
            }
        }
        fscanf(commit_log, "\n");
    }
    return 0;
}

struct dirent *search_in_directory(DIR *directory, char name[], int type)
{
    struct dirent *entry;
    rewinddir(directory);
    while ((entry = readdir(directory)) != NULL)
    {
        if (entry->d_type == type)
        {
            if (strcmp(name, entry->d_name) == 0)
            {
                rewinddir(directory);
                return entry;
            }
        }
    }
    rewinddir(directory);
    return NULL;
}

bool is_similar(char address1[], char address2[], int type)
{
    if (type == DT_REG)
    {
        FILE *file1 = fopen(address1, "rb");
        FILE *file2 = fopen(address2, "rb");
        while (1)
        {
            if ((feof(file2) && !feof(file1)) || (feof(file1) && !feof(file2)))
            {
                fclose(file1);
                fclose(file2);
                return false;
            }

            if (feof(file1) && feof(file2))
            {
                break;
            }

            unsigned char file1_byte;
            fread(&file1_byte, 1, 1, file1);
            unsigned char file2_byte;
            fread(&file2_byte, 1, 1, file2);

            if (file1_byte != file2_byte)
            {
                fclose(file1);
                fclose(file2);
                return false;
            }
        }
        return true;
    }

    else
    {
        DIR *directory1 = opendir(address1);
        struct dirent *entry1;
        DIR *directory2 = opendir(address2);
        struct dirent *entry2;

        while ((entry1 = readdir(directory1)) != NULL)
        {
            if (((entry1->d_type != DT_DIR) && (entry1->d_type != DT_REG)) ||
                (entry1->d_name[0] == '.'))
            {
                continue;
            }

            entry2 = search_in_directory(directory2, entry1->d_name, entry1->d_type);
            if (entry2 == NULL)
            {
                closedir(directory1);
                closedir(directory2);
                return false;
            }

            char file1_address[MAX_PATH_LENGTH] = "";
            sprintf(file1_address, "%s/%s", address1, entry1->d_name);
            char file2_address[MAX_PATH_LENGTH] = "";
            sprintf(file2_address, "%s/%s", address2, entry2->d_name);

            if (!is_similar(file1_address, file2_address, entry1->d_type))
            {
                closedir(directory1);
                closedir(directory2);
                return false;
            }
        }
        return true;
    }
}

bool able_to_checkout()
{
    char last_commit_folder_address[MAX_PATH_LENGTH];
    sprintf(last_commit_folder_address, ".kambiz/branches/%s/%d", current_branch_name, find_branch_head_n_id(current_branch_name, 1));
    DIR *last_commit_folder = opendir(last_commit_folder_address);
    DIR *working_tree = opendir(".");
    DIR *stage = opendir(".kambiz/stage");
    struct dirent *working_tree_entry;
    while ((working_tree_entry = readdir(working_tree)) != NULL)
    {
        if (((working_tree_entry->d_type != DT_DIR) && (working_tree_entry->d_type != DT_REG)) ||
            (working_tree_entry->d_name[0] == '.'))
        {
            continue;
        }

        if (search_in_directory(last_commit_folder, working_tree_entry->d_name, working_tree_entry->d_type) == NULL)
        {
            closedir(last_commit_folder);
            closedir(working_tree);
            closedir(stage);
            return false;
        }
        else
        {
            char last_commit_file_address[MAX_PATH_LENGTH];
            sprintf(last_commit_file_address, "%s/%s", last_commit_folder_address, working_tree_entry->d_name);
            if (!is_similar(working_tree_entry->d_name, last_commit_file_address, working_tree_entry->d_type))
            {
                closedir(last_commit_folder);
                closedir(working_tree);
                closedir(stage);
                return false;
            }
        }
    }

    struct dirent *last_commit_entry;
    while ((last_commit_entry = readdir(last_commit_folder)) != NULL)
    {
        if (((last_commit_entry->d_type != DT_DIR) && (last_commit_entry->d_type != DT_REG)) ||
            (last_commit_entry->d_name[0] == '.'))
        {
            continue;
        }

        if (search_in_directory(working_tree, last_commit_entry->d_name, last_commit_entry->d_type) == NULL)
        {
            closedir(last_commit_folder);
            closedir(working_tree);
            closedir(stage);
            return false;
        }
    }
    closedir(stage);

    return 1;
}

int init()
{
    char temp_address[MAX_PATH_LENGTH];
    strcpy(temp_address, local_address);
    bool already_exists = false;

    DIR *directory = opendir(temp_address);
    while (directory != NULL)
    {
        struct dirent *entry = search_in_directory(directory, ".kambiz", DT_DIR);
        if (entry != NULL)
        {
            fprintf(stderr, "Initialization has been done before in %s\n", temp_address);
            closedir(directory);
            return -1;
        }
        closedir(directory);
        char *last_slash = strrchr(temp_address, '/');
        *last_slash = '\0';
        directory = opendir(temp_address);
    }

    if (mkdir(".kambiz", 0777) == -1)
    {
        fprintf(stderr, "Initialization can't be done: %s\n", strerror(errno));
        return -1;
    }

    char user_address[MAX_PATH_LENGTH] = ".kambiz/user.txt";
    FILE *user_file = fopen(user_address, "w");
    fclose(user_file);

    char aliases_address[MAX_PATH_LENGTH] = ".kambiz/aliases.txt";
    FILE *aliases_file = fopen(aliases_address, "w");
    fclose(aliases_file);

    char hooks_address[MAX_PATH_LENGTH] = ".kambiz/hooks.txt";
    FILE *hooks_file = fopen(hooks_address, "w");
    fprintf(hooks_file, "%-100s%-100s\n", "todo-check", "disabled");
    fprintf(hooks_file, "%-100s%-100s\n", "eof-blank-space", "disabled");
    fprintf(hooks_file, "%-100s%-100s\n", "format-check", "disabled");
    fprintf(hooks_file, "%-100s%-100s\n", "balance-braces", "disabled");
    fprintf(hooks_file, "%-100s%-100s\n", "file-size-check", "disabled");
    fprintf(hooks_file, "%-100s%-100s\n", "character-limit", "disabled");
    fprintf(hooks_file, "%-100s%-100s\n", "static-error-check", "disabled");

    fclose(hooks_file);

    char tags_address[MAX_PATH_LENGTH] = ".kambiz/tags.txt";
    FILE *tags_file = fopen(tags_address, "w");
    fclose(tags_file);

    char deleting_stage_address[MAX_PATH_LENGTH] = ".kambiz/deleting_stage.txt";
    FILE *deleting_stage_file = fopen(deleting_stage_address, "w");
    fclose(deleting_stage_file);

    mkdir(".kambiz/stage", 0777);
    mkdir(".kambiz/branches", 0777);
    mkdir(".kambiz/tags", 0777);
    mkdir(".kambiz/shortcuts", 0777);
    mkdir(".kambiz/branches/master", 0777);
    mkdir(".kambiz/branches/master/0", 0777);

    char current_branch_address[MAX_PATH_LENGTH] = ".kambiz/branches/current_branch.txt";
    FILE *current_branch_file = fopen(current_branch_address, "w");
    fprintf(current_branch_file, "master");
    fclose(current_branch_file);

    char current_id_address[MAX_PATH_LENGTH] = ".kambiz/branches/current_id.txt";
    FILE *current_id_file = fopen(current_id_address, "w");
    fprintf(current_id_file, "0");
    fclose(current_id_file);

    char commit_log_address[MAX_PATH_LENGTH] = ".kambiz/branches/commit_log.txt";
    FILE *commit_log_file = fopen(commit_log_address, "w");
    fclose(commit_log_file);

    char last_commit_id_address[MAX_PATH_LENGTH] = ".kambiz/branches/last_commit_id.txt";
    FILE *last_commit_id_file = fopen(last_commit_id_address, "w");
    fprintf(last_commit_id_file, "0");
    fclose(last_commit_id_file);

    fprintf(stdout, "Kambiz initialization was done sucsessfully at %s\n", local_address);
    return 1;
}

int config(char attribute[], char value[], bool is_global)
{
    char config_address[MAX_PATH_LENGTH] = "";
    if (is_global)
    {
        strcat(config_address, global_address);
    }
    if (!is_global)
    {
        strcat(config_address, ".kambiz");
    }

    char attribute_type[MAX_SHORT_COMMAND_LENGTH] = "";
    char attribute_value[MAX_SHORT_COMMAND_LENGTH] = "";
    split_by_chars(attribute, attribute_type, attribute_value, ".");

    if ((strcmp(attribute_type, "") == 0) || (strcmp(attribute_type, "") == 0))
    {
        fprintf(stderr, "Invalid config command");
        return -1;
    }

    if (strcmp(attribute_type, "user") == 0)
    {
        strcat(config_address, "/user.txt");
        FILE *config_file = fopen(config_address, "r+");

        if (strcmp(attribute_value, "name") == 0)
        {
            char previous_name_value[MAX_STRING_LENGTH] = "";
            get_value_by_attribute(config_file, attribute_value, previous_name_value, 1, 1, false);
            if (strcmp(previous_name_value, "") != 0)
            {
                change_value_by_attribute(config_file, attribute_value, value, 1, 1, false);
            }
            else
            {
                fseek(config_file, 0, SEEK_END);
                fprintf(config_file, "%-100s%-100s\n", attribute_value, value);
            }

            fprintf(stdout, "User's name was successfully assigned to %s", value);
            fclose(config_file);
            return 1;
        }

        else if (strcmp(attribute_value, "email") == 0)
        {
            char previous_email_value[MAX_STRING_LENGTH] = "";
            get_value_by_attribute(config_file, attribute_value, previous_email_value, 1, 1, false);
            if (strcmp(previous_email_value, "") != 0)
            {
                change_value_by_attribute(config_file, attribute_value, value, 1, 1, false);
            }
            else
            {
                fseek(config_file, 0, SEEK_END);
                fprintf(config_file, "%-100s%-100s\n", attribute_value, value);
            }

            fprintf(stdout, "User's email was successfully assigned to %s", value);
            fclose(config_file);
            return 1;
        }

        else
        {
            fprintf(stderr, "Attribute \"%s\" does not exist for users", value);
            fclose(config_file);
            return -1;
        }
    }

    if (strcmp(attribute_type, "alias") == 0)
    {
        strcat(config_address, "/aliases.txt");
        FILE *config_file = fopen(config_address, "r+");
        char previous_alias_value[MAX_STRING_LENGTH] = "";
        get_value_by_attribute(config_file, attribute_value, previous_alias_value, 1, 1, true);
        if (strcmp(previous_alias_value, "") != 0)
        {
            change_value_by_attribute(config_file, attribute_value, value, 1, 1, false);
            fprintf(stdout, "Alias was sucsessfully overwrited: kambiz %s -> %s", attribute_value, value);
        }
        else
        {
            fseek(config_file, 0, SEEK_END);
            printf("%lu", ftell(config_file));
            fprintf(config_file, "%-100s%-100s\n", attribute_value, value);
            fprintf(stdout, "Alias was sucsessfully created: kambiz %s -> %s", attribute_value, value);
        }
        fclose(config_file);
        return 1;
    }

    else
    {
        fprintf(stderr, "Invalid config command");
        return -1;
    }
}

int add(char **file_addresses, int file_count)
{
    bool add_done = false;
    FILE *add_log = fopen(".kambiz/add_log.txt", "a");
    for (int i = 0; i < file_count; i++)
    {
        char *last_slash = strrchr(file_addresses[i], '/');
        char folder_address[MAX_PATH_LENGTH] = ".";
        char file_name[MAX_STRING_LENGTH] = "";
        strcpy(file_name, file_addresses[i]);
        if (last_slash != NULL)
        {
            strcpy(file_name, last_slash + 1);
            strcpy(folder_address, file_addresses[i]);
            last_slash = strrchr(folder_address, '/');
            *last_slash = '\0';
        }

        char stage_folder_address[MAX_PATH_LENGTH] = "";
        sprintf(stage_folder_address, ".kambiz/stage/%s", folder_address);
        DIR *stage_folder = opendir(stage_folder_address);

        if (stage_folder != NULL)
        {
            char staged_file_address[MAX_PATH_LENGTH] = "";
            sprintf(staged_file_address, ".kambiz/stage/%s", file_addresses[i]);
            if ((search_in_directory(stage_folder, file_name, DT_DIR) != NULL) && is_similar(file_addresses[i], staged_file_address, DT_DIR) ||
                (search_in_directory(stage_folder, file_name, DT_REG) != NULL) && is_similar(file_addresses[i], staged_file_address, DT_REG))
            {
                fprintf(stderr, "\"%s\" is already staged\n", file_addresses[i]);
                continue;
            }
            closedir(stage_folder);
        }

        DIR *working_tree_folder = opendir(folder_address);
        struct dirent *entry;
        entry = search_in_directory(working_tree_folder, file_name, DT_DIR);
        if (entry == NULL)
        {
            entry = search_in_directory(working_tree_folder, file_name, DT_REG);
        }

        if (entry != NULL)
        {
            add_done = true;
            char command[MAX_FULL_COMMAND_LENGTH] = "";
            sprintf(command, "cp -r \"%s\" \".kambiz/stage/%s/\" 2>/dev/null", file_addresses[i], folder_address);
            if (system(command) != 0)
            {
                char pre_command[MAX_FULL_COMMAND_LENGTH] = "";
                sprintf(pre_command, "mkdir -p \".kambiz/stage/%s\"", folder_address);
                system(pre_command);
                system(command);
            }
            fprintf(stdout, "\"%s\" was successfully staged\n", file_addresses[i]);
            closedir(working_tree_folder);
            continue;
        }

        closedir(working_tree_folder);

        char last_commit_folder_address[MAX_PATH_LENGTH];
        sprintf(last_commit_folder_address, ".kambiz/branches/%s/%d/%s", current_branch_name, find_branch_head_n_id(current_branch_name, 1), folder_address);
        DIR *last_commit_folder = opendir(last_commit_folder_address);
        if ((search_in_directory(last_commit_folder, file_name, DT_DIR) != NULL) ||
            (search_in_directory(last_commit_folder, file_name, DT_REG) != NULL))
        {
            bool is_staged = false;
            FILE *deleting_stage = fopen(".kambiz/deleting_stage.txt", "r+");
            char deleting_file_address[MAX_PATH_LENGTH];
            while (fscanf(deleting_stage, "%[^\n]s", deleting_file_address) != EOF)
            {
                if (strcmp(deleting_file_address, file_addresses[i]) == 0)
                {
                    fprintf(stderr, "\"%s\" is already staged to be removed\n", file_addresses[i]);
                    fclose(deleting_stage);
                    is_staged = true;
                    break;
                }
            }

            if (!is_staged)
            {
                fseek(deleting_stage, 0, SEEK_END);
                fprintf(deleting_stage, "%s\n", file_addresses[i]);
                fclose(deleting_stage);
                closedir(last_commit_folder);
                fprintf(stdout, "\"%s\" was successfully staged to be removed\n", file_addresses[i]);
            }

            continue;
        }

        closedir(last_commit_folder);
        fprintf(stderr, "\"%s\" not found\n", file_addresses[i]);
    }
    return 1;
}

int add_redo()
{
    bool redo_done = false;
    DIR *stage = opendir(".kambiz/stage");
    DIR *working_tree = opendir(".");
    struct dirent *entry;
    while ((entry = readdir(stage)) != NULL)
    {
        if (((entry->d_type != DT_DIR) && (entry->d_type != DT_REG)) ||
            (entry->d_name[0] == '.'))
        {
            continue;
        }

        if (search_in_directory(working_tree, entry->d_name, entry->d_type) != NULL)
        {
            char staged_file_address[1000];
            sprintf(staged_file_address, ".kambiz/stage/%s", entry->d_name);
            if (!is_similar(entry->d_name, staged_file_address, entry->d_type))
            {
                char command[MAX_FULL_COMMAND_LENGTH] = "";
                sprintf(command, "cp -r \"%s\" \".kambiz/stage/\"", entry->d_name);
                system(command);
                fprintf(stdout, "\"%s\" ", entry->d_name);
                redo_done = true;
            }
        }
    }

    if (redo_done)
    {
        fprintf(stdout, "were successfully updated on staging area");
    }

    else
    {
        fprintf(stdout, "No not updated files were found on staging area");
    }
    return 1;
}

bool add_n(char address[], int current_depth, int max_depth, char output[])
{
    DIR *working_tree_folder = opendir(address);
    struct dirent *entry;
    bool all_staged = true;
    while ((entry = readdir(working_tree_folder)) != NULL)
    {
        if (((entry->d_type != DT_DIR) && (entry->d_type != DT_REG)) ||
            (entry->d_name[0] == '.'))
        {
            continue;
        }

        if ((current_depth == max_depth) || (entry->d_type == DT_REG))
        {
            bool is_staged = false;

            char file_address[MAX_PATH_LENGTH];
            sprintf(file_address, "%s/%s", address, entry->d_name);

            char staged_file_address[MAX_PATH_LENGTH];
            sprintf(staged_file_address, ".kambiz/stage/%s/%s", address, entry->d_name);

            char staged_folder_address[MAX_PATH_LENGTH];
            sprintf(staged_folder_address, ".kambiz/stage/%s", address);
            DIR *staged_folder = opendir(staged_folder_address);

            if (staged_folder != NULL)
            {
                if (search_in_directory(staged_folder, entry->d_name, entry->d_type) != NULL)
                {
                    if (is_similar(file_address, staged_file_address, entry->d_type))
                    {
                        is_staged = true;
                    }
                }
            }

            if (!is_staged)
            {
                all_staged = false;
            }

            char line[MAX_STRING_LENGTH] = "";
            for (int i = 1; i < current_depth; i++)
            {
                strcat(line, " ");
            }

            if (current_depth != 1)
            {
                strcat(line, "└─");
            }
            strcat(line, entry->d_name);
            if (is_staged)
            {
                strcat(line, " Staged\n");
            }
            if (!is_staged)
            {
                strcat(line, " Not Staged\n");
            }

            char new_output[10000] = "";
            strcat(new_output, line);
            strcat(new_output, output);
            strcpy(output, new_output);
            continue;
        }

        if (entry->d_type == DT_DIR)
        {
            char new_address[MAX_PATH_LENGTH] = "";
            sprintf(new_address, "%s/%s", address, entry->d_name);
            bool is_staged = add_n(new_address, current_depth + 1, max_depth, output);

            char line[MAX_STRING_LENGTH] = "";
            for (int i = 1; i < current_depth; i++)
            {
                strcat(line, " ");
            }
            if (current_depth != 1)
            {
                strcat(line, "└─");
            }
            strcat(line, entry->d_name);
            if (is_staged)
            {
                strcat(line, " Staged\n");
            }
            if (!is_staged)
            {
                strcat(line, " Not Staged\n");
            }

            char new_output[10000] = "";
            strcat(new_output, line);
            strcat(new_output, output);
            strcpy(output, new_output);
        }
    }

    closedir(working_tree_folder);
    return all_staged;
}

int reset(char **file_addresses, int file_count)
{
    for (int i = 0; i < file_count; i++)
    {
        char *last_slash = strrchr(file_addresses[i], '/');
        char folder_address[MAX_PATH_LENGTH] = ".";
        char file_name[MAX_STRING_LENGTH] = "";
        strcpy(file_name, file_addresses[i]);
        if (last_slash != NULL)
        {
            strcpy(file_name, last_slash + 1);
            strcpy(folder_address, file_addresses[i]);
            last_slash = strrchr(folder_address, '/');
            *last_slash = '\0';
        }

        if (file_name[0] == '.')
        {
            fprintf(stdout, "Stop it. Get some help.");
            continue;
        }

        char stage_folder_address[MAX_PATH_LENGTH] = "";
        sprintf(stage_folder_address, ".kambiz/stage/%s", folder_address);
        DIR *stage_folder = opendir(stage_folder_address);

        if (stage_folder == NULL)
        {
            fprintf(stderr, "\"%s\" not found in staging area\n", file_addresses[i]);
            closedir(stage_folder);
            continue;
        }

        if (stage_folder != NULL)
        {
            char staged_file_address[MAX_PATH_LENGTH] = "";
            sprintf(staged_file_address, ".kambiz/stage/%s", file_addresses[i]);
            if ((search_in_directory(stage_folder, file_name, DT_DIR) == NULL) &
                (search_in_directory(stage_folder, file_name, DT_REG) == NULL))
            {
                fprintf(stderr, "\"%s\" not found in staging area\n", file_addresses[i]);
                closedir(stage_folder);
                continue;
            }

            else
            {
                char command[MAX_FULL_COMMAND_LENGTH] = "";
                sprintf(command, "rm -r \".kambiz/stage/%s\"", file_addresses[i]);
                system(command);
                fprintf(stderr, "%s was successfully deleted from staging area\n", file_addresses[i]);
                closedir(stage_folder);
                continue;
            }
        }
    }
    return 1;
}

int reset_undo()
{
    DIR *stage = opendir(".kambiz/stage");
    struct dirent *entry;
    time_t last_time = 0;
    struct dirent *last_entry;

    rewinddir(stage);
    while ((entry = readdir(stage)) != NULL)
    {
        if (((entry->d_type != DT_DIR) && (entry->d_type != DT_REG)) ||
            (entry->d_name[0] == '.'))
        {
            continue;
        }

        char staged_file_address[MAX_PATH_LENGTH];
        sprintf(staged_file_address, ".kambiz/stage/%s", entry->d_name);

        struct stat filedata;
        stat(staged_file_address, &filedata);

        printf("%lu\n", filedata.st_mtime);

        if (filedata.st_mtime > last_time)
        {
            last_entry = entry;
            last_time = filedata.st_mtime;
        }
    }

    char command[MAX_FULL_COMMAND_LENGTH];
    sprintf(command, "rm -r .kambiz/stage/%s", last_entry->d_name);
    system(command);

    fprintf(stdout, "\"%s\" was successfully removed from staging area", last_entry->d_name);
    return 1;
}

int status()
{
    char last_commit_folder_address[MAX_PATH_LENGTH];
    sprintf(last_commit_folder_address, ".kambiz/branches/%s/%d", current_branch_name, find_branch_head_n_id(current_branch_name, 1));
    DIR *last_commit_folder = opendir(last_commit_folder_address);
    DIR *working_tree = opendir(".");
    DIR *stage = opendir(".kambiz/stage");
    struct dirent *working_tree_entry;
    while ((working_tree_entry = readdir(working_tree)) != NULL)
    {
        if (((working_tree_entry->d_type != DT_DIR) && (working_tree_entry->d_type != DT_REG)) ||
            (working_tree_entry->d_name[0] == '.'))
        {
            continue;
        }

        char status = ' ';
        char stage_status;

        if (search_in_directory(last_commit_folder, working_tree_entry->d_name, working_tree_entry->d_type) == NULL)
        {
            status = 'A';
            stage_status = '-';
            if (search_in_directory(stage, working_tree_entry->d_name, working_tree_entry->d_type) != NULL)
            {
                char staged_file_address[MAX_PATH_LENGTH];
                sprintf(staged_file_address, "%s/%s", ".kambiz/stage", working_tree_entry->d_name);
                if (is_similar(working_tree_entry->d_name, staged_file_address, working_tree_entry->d_type))
                {
                    stage_status = '+';
                }
            }
        }
        else
        {
            char last_commit_file_address[MAX_PATH_LENGTH];
            sprintf(last_commit_file_address, "%s/%s", last_commit_folder_address, working_tree_entry->d_name);
            if (!is_similar(working_tree_entry->d_name, last_commit_file_address, working_tree_entry->d_type))
            {
                status = 'M';
                stage_status = '-';

                if (search_in_directory(stage, working_tree_entry->d_name, working_tree_entry->d_type) != NULL)
                {
                    char staged_file_address[MAX_PATH_LENGTH];
                    sprintf(staged_file_address, "%s/%s", ".kambiz/stage", working_tree_entry->d_name);
                    if (is_similar(working_tree_entry->d_name, staged_file_address, working_tree_entry->d_type))
                    {
                        stage_status = '+';
                    }
                }
            }
        }

        if (status != ' ')
        {
            fprintf(stdout, "%s: %c%c\n", working_tree_entry->d_name, stage_status, status);
        }
    }

    struct dirent *last_commit_entry;
    while ((last_commit_entry = readdir(last_commit_folder)) != NULL)
    {
        if (((last_commit_entry->d_type != DT_DIR) && (last_commit_entry->d_type != DT_REG)) ||
            (last_commit_entry->d_name[0] == '.'))
        {
            continue;
        }

        if (search_in_directory(working_tree, last_commit_entry->d_name, last_commit_entry->d_type) == NULL)
        {
            char stage_status = '-';
            FILE *deleting_stage = fopen(".kambiz/deleting_stage.txt", "r");
            char deleting_file_address[MAX_PATH_LENGTH];
            while (fscanf(deleting_stage, "%[^\n]s", deleting_file_address) != EOF)
            {
                if (strcmp(deleting_file_address, last_commit_entry->d_name) == 0)
                {
                    stage_status = '+';
                }
                fscanf(deleting_stage, "\n");
            }
            fprintf(stdout, "%s: %cD\n", last_commit_entry->d_name, stage_status);
        }
    }
    return 1;
}

int commit(char message[])
{
    if (!on_head)
    {
        fprintf(stderr, "Commits can only be done from branch's head");
        return -1;
    }

    if (strlen(message) > 72)
    {
        fprintf(stderr, "Message's charachters can't exceed 72");
        return -1;
    }

    int commit_id = last_commit_id + 1;
    int current_branch_head_id = find_branch_head_n_id(current_branch_name, 1);
    int commited_files_count = 0;
    DIR *stage_folder = opendir(".kambiz/stage");
    struct dirent *entry;
    char new_commit_address[MAX_PATH_LENGTH];
    while ((entry = readdir(stage_folder)) != NULL)
    {
        if (((entry->d_type != DT_DIR) && (entry->d_type != DT_REG)) ||
            (entry->d_name[0] == '.'))
        {
            continue;
        }

        if (commited_files_count == 0)
        {
            sprintf(new_commit_address, ".kambiz/branches/%s/%d", current_branch_name, commit_id);
            mkdir(new_commit_address, 0777);

            char command[MAX_FULL_COMMAND_LENGTH] = "";
            sprintf(command, "cp -r .kambiz/branches/%s/%d/ .kambiz/branches/%s/%d", current_branch_name, current_branch_head_id, current_branch_name, commit_id);
            system(command);

            sprintf(command, "cp -r .kambiz/stage/ .kambiz/branches/%s/%d", current_branch_name, commit_id);
            system(command);

            sprintf(command, "rm -r .kambiz/stage/*");
            system(command);
        }

        commited_files_count++;
    }

    FILE *deleting_stage = fopen(".kambiz/deleting_stage.txt", "r");
    char deleting_file_address[MAX_PATH_LENGTH];
    while (fscanf(deleting_stage, "%[^\n]s", deleting_file_address) != EOF)
    {
        if (commited_files_count == 0)
        {
            sprintf(new_commit_address, ".kambiz/branches/%s/%d", current_branch_name, commit_id);
            mkdir(new_commit_address, 0777);

            char command[MAX_FULL_COMMAND_LENGTH] = "";
            sprintf(command, "cp -r .kambiz/branches/%s/%d/ .kambiz/branches/%s/%d", current_branch_name, current_branch_head_id, current_branch_name, commit_id);
            system(command);
        }

        char command[MAX_FULL_COMMAND_LENGTH] = "";
        sprintf(command, "rm -r \"%s/%s\"", new_commit_address, deleting_file_address);
        system(command);

        commited_files_count++;
        fscanf(deleting_stage, "\n");
    }

    deleting_stage = fopen(".kambiz/deleting_stage.txt", "w");
    fclose(deleting_stage);

    if (commited_files_count == 0)
    {
        fprintf(stderr, "No staged files to commit");
        return -1;
    }

    time_t current_time = time(NULL);
    char current_time_string[MAX_STRING_LENGTH];
    strftime(current_time_string, sizeof(current_time_string), "%Y/%m/%d-%H:%M:%S", localtime(&current_time));

    fprintf(stdout, "%d staged files successfully commited at %s (Commit-ID: %d)", commited_files_count, current_time_string, commit_id);
    char commit_log_line[10000] = "";
    sprintf(commit_log_line, "%-100d%-100s%-100s%-100s%-100d%s\n", commit_id, current_branch_name, user_name, current_time_string, commited_files_count, message);

    FILE *commit_log_file = fopen(".kambiz/branches/commit_log.txt", "r");
    char commit_log_content[10000];
    strcpy(commit_log_content, commit_log_line);
    char new_char;
    while ((new_char = fgetc(commit_log_file)) != EOF)
    {
        strncat(commit_log_content, &new_char, 1);
    }
    fclose(commit_log_file);

    commit_log_file = fopen(".kambiz/branches/commit_log.txt", "w");
    fprintf(commit_log_file, "%s", commit_log_content);
    fclose(commit_log_file);

    FILE *last_commit_id_file = fopen(".kambiz/branches/last_commit_id.txt", "w");
    fprintf(last_commit_id_file, "%d", commit_id);
    fclose(last_commit_id_file);

    FILE *current_id_file = fopen(".kambiz/branches/current_id.txt", "w");
    fprintf(current_id_file, "%d", commit_id);
    fclose(current_id_file);

    return 1;
}

int set_shortcut(char shortcut_name[], char shortcut_message[])
{
    char shortcut_file_address[MAX_PATH_LENGTH];
    sprintf(shortcut_file_address, ".kambiz/shortcuts/%s.txt", shortcut_name);
    FILE *shortcut_file = fopen(shortcut_file_address, "w");
    fprintf(shortcut_file, "%s\n", shortcut_message);
    fclose(shortcut_file);

    fprintf(stdout, "Message shortcut was successfully created (%s -> %s)", shortcut_name, shortcut_message);
    return 1;
}

int replace_shortcut(char shortcut_name[], char shortcut_message[])
{
    char shortcut_file_address[MAX_PATH_LENGTH];
    sprintf(shortcut_file_address, ".kambiz/shortcuts/%s.txt", shortcut_name);
    FILE *shortcut_file = fopen(shortcut_file_address, "r");
    if (shortcut_file == NULL)
    {
        fprintf(stderr, "No shortcut exists with this name");
        return -1;
    }
    fclose(shortcut_file);
    shortcut_file = fopen(shortcut_file_address, "w");
    fprintf(shortcut_file, "%s\n", shortcut_message);
    fclose(shortcut_file);

    fprintf(stdout, "Message shortcut was successfully overwrited (%s -> %s)", shortcut_name, shortcut_message);
    return 1;
}

int remove_shortcut(char shortcut_name[])
{
    char shortcut_file_address[MAX_PATH_LENGTH];
    sprintf(shortcut_file_address, ".kambiz/shortcuts/%s.txt", shortcut_name);
    FILE *shortcut_file = fopen(shortcut_file_address, "r");
    if (shortcut_file == NULL)
    {
        fprintf(stderr, "No shortcut exists with this name");
        return -1;
    }
    fclose(shortcut_file);

    char command[MAX_FULL_COMMAND_LENGTH] = "";
    sprintf(command, "rm -r \"%s\"", shortcut_file_address);
    system(command);

    fprintf(stdout, "Message shortcut was successfully deleted (%s)", shortcut_name);
    return 1;
}

int branch(char new_branch_name[])
{
    char new_branch_address[MAX_PATH_LENGTH];
    sprintf(new_branch_address, ".kambiz/branches/%s", new_branch_name);
    mkdir(new_branch_address, 0777);

    char new_branch_start_address[MAX_PATH_LENGTH];
    sprintf(new_branch_start_address, ".kambiz/branches/%s/0", new_branch_name);
    mkdir(new_branch_start_address, 0777);

    int current_branch_head_id = find_branch_head_n_id(current_branch_name, 1);

    char command[MAX_FULL_COMMAND_LENGTH] = "";
    sprintf(command, "cp -r .kambiz/branches/%s/%d/ .kambiz/branches/%s/0", current_branch_name, current_branch_head_id, new_branch_name);
    system(command);

    fprintf(stdout, "Branch %s was successfully created", new_branch_name);

    return 1;
}

int show_branches()
{
    DIR *branches_folder = opendir(".kambiz/branches");
    struct dirent *entry;
    while ((entry = readdir(branches_folder)) != NULL)
    {
        if ((entry->d_name[0] == '.') || (entry->d_type != DT_DIR))
        {
            continue;
        }
        printf("%s", entry->d_name);
        if (strcmp(entry->d_name, current_branch_name) == 0)
        {
            printf("(Current)");
        }
        printf("\n");
    }

    return 1;
}

int checkout_commit(char branch_name[], int id)
{
    if ((!able_to_checkout()) && (on_head))
    {
        fprintf(stderr, "Can't checkout: Not all changes, deletions or additions are commited");
        return -1;
    }

    DIR *working_tree = opendir(".");
    struct dirent *entry;
    while ((entry = readdir(working_tree)) != NULL)
    {
        if (((entry->d_type != DT_DIR) && (entry->d_type != DT_REG)) ||
            (entry->d_name[0] == '.'))
        {
            continue;
        }

        char command[MAX_FULL_COMMAND_LENGTH] = "";
        sprintf(command, "rm -r \"%s\"", entry->d_name);
        system(command);
    }

    char command[MAX_FULL_COMMAND_LENGTH] = "";
    sprintf(command, "cp -r .kambiz/branches/%s/%d/ .", branch_name, id);
    system(command);

    sprintf(command, "rm -r .kambiz/stage/* 2>/dev/null");
    system(command);

    FILE *current_branch_file = fopen(".kambiz/branches/current_branch.txt", "w");
    fprintf(current_branch_file, "%s", branch_name);
    fclose(current_branch_file);

    FILE *current_id_file = fopen(".kambiz/branches/current_id.txt", "w");
    fprintf(current_id_file, "%d", id);
    fclose(current_id_file);

    fprintf(stdout, "Checkout to commit-id: %d on %s branch was successfully done\n", id, branch_name);
    return 1;
}

int revert(char branch_name[], int id, char message[])
{
    checkout_commit(branch_name, id);

    int commit_id = last_commit_id + 1;
    int commited_files_count = 0;

    char new_commit_address[MAX_PATH_LENGTH];
    sprintf(new_commit_address, ".kambiz/branches/%s/%d", current_branch_name, commit_id);
    mkdir(new_commit_address, 0777);

    char command[MAX_FULL_COMMAND_LENGTH] = "";
    sprintf(command, "cp -r .kambiz/branches/%s/%d/ .kambiz/branches/%s/%d", current_branch_name, id, current_branch_name, commit_id);
    system(command);

    sprintf(command, "cp -r .kambiz/stage/ .kambiz/branches/%s/%d", current_branch_name, commit_id);
    system(command);

    time_t current_time = time(NULL);
    char current_time_string[MAX_STRING_LENGTH];
    strftime(current_time_string, sizeof(current_time_string), "%Y/%m/%d-%H:%M:%S", localtime(&current_time));

    fprintf(stdout, "Revert successfully done at %s (Commit-ID: %d)", current_time_string, commit_id);
    char commit_log_line[10000] = "";
    sprintf(commit_log_line, "%-100d%-100s%-100s%-100s%-100d%s\n", commit_id, current_branch_name, user_name, current_time_string, commited_files_count, message);

    FILE *commit_log_file = fopen(".kambiz/branches/commit_log.txt", "r");
    char commit_log_content[10000];
    strcpy(commit_log_content, commit_log_line);
    char new_char;
    while ((new_char = fgetc(commit_log_file)) != EOF)
    {
        strncat(commit_log_content, &new_char, 1);
    }
    fclose(commit_log_file);

    commit_log_file = fopen(".kambiz/branches/commit_log.txt", "w");
    fprintf(commit_log_file, "%s", commit_log_content);
    fclose(commit_log_file);

    FILE *last_commit_id_file = fopen(".kambiz/branches/last_commit_id.txt", "w");
    fprintf(last_commit_id_file, "%d", commit_id);
    fclose(last_commit_id_file);

    FILE *current_id_file = fopen(".kambiz/branches/current_id.txt", "w");
    fprintf(current_id_file, "%d", commit_id);
    fclose(current_id_file);

    return 1;
}

int log_filter(char option[], char **arguments, int arguments_count)
{
    FILE *commit_log_file = fopen(".kambiz/branches/commit_log.txt", "r");
    char line[601];

    bool filter_n = (strcmp(option, "-n") == 0);
    bool filter_author = (strcmp(option, "-author") == 0);
    bool filter_branch = (strcmp(option, "-branch") == 0);
    bool filter_since = (strcmp(option, "-since") == 0);
    bool filter_before = (strcmp(option, "-before") == 0);
    bool filter_word = (strcmp(option, "-search") == 0);

    int n;
    int log_counter;
    if (filter_n)
    {
        log_counter = 0;
        sscanf(arguments[0], "%d", &n);
    }

    time_t wanted_time;
    if (filter_since || filter_before)
    {
        wanted_time = string_to_time(arguments[0]);
        if (wanted_time == -1)
        {
            fprintf(stderr, "Time should be in YYYY/mm/dd HH:MM:SS format");
            return -1;
        }
    }

    while (fscanf(commit_log_file, "%[^\n]s", line) != EOF)
    {
        char id[101];
        char author[101];
        char branch[101];
        char time[101];
        char message[101];
        char commited_files_count[101];
        sscanf(line, "%100s", id);
        sscanf(line + 100, "%100s", branch);
        sscanf(line + 200, "%100s", author);
        sscanf(line + 300, "%100s", time);
        sscanf(line + 400, "%100s", commited_files_count);
        sscanf(line + 500, "%[^\n]s", message);

        if (filter_n)
        {
            log_counter++;
            if (log_counter > n)
            {
                break;
            }
        }
        if (filter_author)
        {
            if (strcmp(author, arguments[0]) != 0)
            {
                fscanf(commit_log_file, "\n");
                continue;
            }
        }
        if (filter_branch)
        {
            if (strcmp(branch, arguments[0]) != 0)
            {
                fscanf(commit_log_file, "\n");
                continue;
            }
        }
        if (filter_since)
        {
            if (string_to_time(time) < wanted_time)
            {
                fscanf(commit_log_file, "\n");
                continue;
            }
        }
        if (filter_before)
        {
            if (string_to_time(time) > wanted_time)
            {
                fscanf(commit_log_file, "\n");
                continue;
            }
        }
        if (filter_word)
        {
            bool found_word = false;
            for (int i = 0; i < arguments_count; i++)
            {
                if (strstr(message, arguments[i]) != NULL)
                {
                    found_word = true;
                    break;
                }
            }
            if (!found_word)
            {
                fscanf(commit_log_file, "\n");
                continue;
            }
        }
        printf("Commit-ID: %s | Branch: %s | Author: %s | Time: %s | Files_count: %s | Message: \"%s\"\n",
               id, branch, author, time, commited_files_count, message);
        fscanf(commit_log_file, "\n");
    }
    return 1;
}

int file_diff(char address1[], char address2[], int begin1, int end1, int begin2, int end2)
{
    FILE *file1 = fopen(address1, "r");
    char file1_content[MAX_FILE_SIZE] = "";
    char temp_char;
    while ((temp_char = fgetc(file1)) != EOF)
    {
        strncat(file1_content, &temp_char, 1);
    }
    fclose(file1);

    FILE *file2 = fopen(address2, "r");
    char file2_content[MAX_FILE_SIZE] = "";
    while ((temp_char = fgetc(file2)) != EOF)
    {
        strncat(file2_content, &temp_char, 1);
    }
    fclose(file2);

    char file1_lines[1000][MAX_STRING_LENGTH];
    char line[MAX_STRING_LENGTH];
    int file1_line_counter = 0;
    int file1_position = 0;
    file1_position += strspn(file1_content + file1_position, "\t\n ");
    while (sscanf(file1_content + file1_position, "%[^\n]s", line) != -1)
    {
        file1_position += strcspn(file1_content + file1_position, "\n");
        file1_position += strspn(file1_content + file1_position, "\t\n ");
        strcpy(file1_lines[file1_line_counter++], line);
    }

    char file2_lines[1000][MAX_STRING_LENGTH];
    int file2_line_counter = 0;
    int file2_position = 0;
    file2_position += strspn(file2_content + file2_position, "\t\n ");
    while (sscanf(file2_content + file2_position, "%[^\n]s", line) != -1)
    {
        file2_position += strcspn(file2_content + file2_position, "\n");
        file2_position += strspn(file2_content + file2_position, "\t\n ");
        strcpy(file2_lines[file2_line_counter++], line);
    }

    for (int i1 = begin1 - 1, i2 = begin2 - 1; (((i1 < end1) && (i1 < file1_line_counter)) || ((i2 < end2) && (i2 < file2_line_counter))); i1++, i2++)
    {
        if ((i1 >= file1_line_counter) || (i1 >= end1))
        {
            fprintf(stdout, "-------\n");
            fprintf(stdout, "File %s - Line %d:\n\n", address1, i1 + 1);
            fprintf(stdout, "File %s - Line %d:\n" CYAN "%s" COLOR_RESET "\n", address2, i2 + 1, file2_lines[i2]);
            fprintf(stdout, "-------\n");
            continue;
        }

        if ((i2 >= file2_line_counter) || (i2 >= end2))
        {
            fprintf(stdout, "-------\n");
            fprintf(stdout, "File %s - Line %d:\n" YELLOW "%s" COLOR_RESET "\n", address1, i1 + 1, file1_lines[i1]);
            fprintf(stdout, "File %s - Line %d:\n\n", address2, i2 + 1);
            fprintf(stdout, "-------\n");
            continue;
        }

        if (strcmp(file1_lines[i1], file2_lines[i2]) != 0)
        {
            fprintf(stdout, "-------\n");
            fprintf(stdout, "File %s - Line %d:\n" YELLOW "%s" COLOR_RESET "\n", address1, i1 + 1, file1_lines[i1]);
            fprintf(stdout, "File %s - Line %d:\n" CYAN "%s" COLOR_RESET "\n", address2, i2 + 1, file2_lines[i2]);
            fprintf(stdout, "-------\n");
        }
    }

    return 1;
}

int commit_diff(char branch_name1[], char string_id1[], char branch_name2[], char string_id2[])
{
    char commit1_folder_address[MAX_PATH_LENGTH];
    sprintf(commit1_folder_address, ".kambiz/branches/%s/%s", branch_name1, string_id1);
    DIR *commit1_folder = opendir(commit1_folder_address);
    struct dirent *commit1_entry;

    char commit2_folder_address[MAX_PATH_LENGTH];
    sprintf(commit2_folder_address, ".kambiz/branches/%s/%s", branch_name2, string_id2);
    DIR *commit2_folder = opendir(commit2_folder_address);
    struct dirent *commit2_entry;

    while ((commit1_entry = readdir(commit1_folder)) != NULL)
    {
        if ((commit1_entry->d_name[0] == '.') || (commit1_entry->d_type != DT_REG))
        {
            continue;
        }

        if (search_in_directory(commit2_folder, commit1_entry->d_name, DT_REG) == NULL)
        {
            fprintf(stdout, "%s exists in %s, but not in %s\n", commit1_entry->d_name, string_id1, string_id2);
        }

        else
        {
            char commit1_file_address[MAX_PATH_LENGTH];
            sprintf(commit1_file_address, "%s/%s", commit1_folder_address, commit1_entry->d_name);

            char commit2_file_address[MAX_PATH_LENGTH];
            sprintf(commit2_file_address, "%s/%s", commit2_folder_address, commit1_entry->d_name);

            file_diff(commit1_file_address, commit2_file_address, 1, MAX_INT_VALUE, 1, MAX_INT_VALUE);
        }
    }

    while ((commit2_entry = readdir(commit2_folder)) != NULL)
    {
        if ((commit2_entry->d_name[0] == '.') || (commit2_entry->d_type != DT_REG))
        {
            continue;
        }

        if (search_in_directory(commit1_folder, commit2_entry->d_name, DT_REG) == NULL)
        {
            fprintf(stdout, "%s exists in %s, but not in %s\n", commit2_entry->d_name, string_id2, string_id1);
        }
    }

    return 1;
}

int add_tag(char tag_name[], char string_id[], char branch_name[], char tag_message[], bool dash_f)
{
    char tag_file_address[MAX_PATH_LENGTH];
    sprintf(tag_file_address, ".kambiz/tags/%s.txt", tag_name);
    FILE *tag_file = fopen(tag_file_address, "r");
    bool already_exists = false;
    if (tag_file != NULL)
    {
        already_exists = true;
        if (!dash_f)
        {
            fprintf(stderr, "A tag already exists with this name");
            return -1;
        }
    }
    fclose(tag_file);

    time_t current_time = time(NULL);
    char current_time_string[MAX_STRING_LENGTH];
    strftime(current_time_string, sizeof(current_time_string), "%Y/%m/%d-%H:%M:%S", localtime(&current_time));

    tag_file = fopen(tag_file_address, "w");
    fprintf(tag_file, "Tag Name: %s\n", tag_name);
    fprintf(tag_file, "Commit ID: %s\n", string_id);
    fprintf(tag_file, "Author: %s %s\n", user_name, user_email);
    fprintf(tag_file, "Time: %s\n", current_time_string);
    fprintf(tag_file, "Message: %s\n", tag_message);
    fclose(tag_file);

    if (!already_exists)
    {
        fprintf(stdout, "Tag %s was successfully created (Commit-ID: %s)", tag_name, string_id);
    }

    else
    {
        fprintf(stdout, "Tag %s was successfully overwrited (Commit-ID: %s)", tag_name, string_id);
    }

    return 1;
}

int show_tag(char tag_name[])
{
    char tag_file_address[MAX_PATH_LENGTH];
    sprintf(tag_file_address, ".kambiz/tags/%s.txt", tag_name);
    FILE *tag_file = fopen(tag_file_address, "r");
    if (tag_file == NULL)
    {
        fprintf(stderr, "No tag exists with this name");
        return -1;
    }

    char line[MAX_STRING_LENGTH];
    while (fgets(line, MAX_STRING_LENGTH, tag_file) != NULL)
    {
        fprintf(stdout, "%s", line);
    }

    fclose(tag_file);
    return 1;
}

int list_tags()
{
    DIR *tags_folder = opendir(".kambiz/tags");
    struct dirent *entry;
    int tags_count = 0;
    char tags[100][MAX_STRING_LENGTH];

    while ((entry = readdir(tags_folder)) != NULL)
    {
        if ((entry->d_name[0] == '.') || (entry->d_type != DT_REG))
        {
            continue;
        }
        strcpy(tags[tags_count++], entry->d_name);
    }

    for (int i = 0; i < tags_count; i++)
    {

        for (int j = 0; j < tags_count - i - 1; j++)
        {
            if (strcmp(tags[j], tags[j + 1]) > 0)
            {
                char temp[MAX_STRING_LENGTH];
                strcpy(temp, tags[j]);
                strcpy(tags[j], tags[j + 1]);
                strcpy(tags[j + 1], temp);
            }
        }
    }

    for (int i = 0; i < tags_count; i++)
    {
        printf("%s\n", tags[i]);
    }

    return 1;
}

int grep(char file_name[], char wanted_word[], char id_string[], char branch_name[], bool dash_n)
{
    char file_address[MAX_PATH_LENGTH];
    sprintf(file_address, ".kambiz/branches/%s/%s/%s", branch_name, id_string, file_name);
    FILE *file = fopen(file_address, "r");

    if (file == NULL)
    {
        fprintf(stderr, "File not found!");
        return -1;
    }

    char line[MAX_STRING_LENGTH];
    int line_count = 0;
    while (fgets(line, MAX_STRING_LENGTH, file) != NULL)
    {
        line_count++;
        char *word_position = strstr(line, wanted_word);
        if (word_position != NULL)
        {
            char previous_part[MAX_STRING_LENGTH];
            strncpy(previous_part, line, word_position - line);

            char next_part[MAX_STRING_LENGTH];
            strcpy(word_position + strlen(wanted_word) + 1, next_part);

            if (dash_n)
            {
                fprintf(stdout, "%d ", line_count);
            }
            fprintf(stdout, "%s" CYAN "%s" COLOR_RESET "%s\n", previous_part, wanted_word, next_part);
        }
    }

    return 1;
}

int list_hooks()
{
    FILE *hooks_file = fopen(".kambiz/hooks.txt", "r");
    char line[202];
    while (fgets(line, 202, hooks_file) != NULL)
    {
        char hook_id[101];
        char state[101];
        sscanf(line, "%100s", hook_id);
        printf("%s\n", hook_id);
    }
    fclose(hooks_file);
    return 1;
}

int list_applied_hooks()
{
    FILE *hooks_file = fopen(".kambiz/hooks.txt", "r");
    char line[202];
    while (fgets(line, 202, hooks_file) != NULL)
    {
        char hook_id[101];
        char state[101];
        sscanf(line, "%100s", hook_id);
        sscanf(line + 100, "%100s", state);
        if (strcmp(state, "enabled") == 0)
        {
            printf("%s\n", hook_id);
        }
    }
    fclose(hooks_file);
    return 1;
}

int add_hook(char hook_id[])
{
    FILE *hooks_file = fopen(".kambiz/hooks.txt", "r+");
    change_value_by_attribute(hooks_file, hook_id, "enabled", 2, 1, false);
    fclose(hooks_file);
    fprintf(stdout, "%s hook was enabled succesfully\n", hook_id);
    return 1;
}

int remove_hook(char hook_id[])
{
    FILE *hooks_file = fopen(".kambiz/hooks.txt", "r+");
    change_value_by_attribute(hooks_file, hook_id, "disabled", 2, 1, false);
    fclose(hooks_file);
    fprintf(stdout, "%s hook was disabled succesfully\n", hook_id);
    return 1;
}

int pre_commit()
{
    FILE *hooks_file = fopen(".kambiz/hooks.txt", "r");
    char line[202];
    char hook_list[10][100];
    int hooks_count = 0;
    while (fgets(line, 202, hooks_file) != NULL)
    {
        char hook_id[101];
        char state[101];
        sscanf(line, "%100s", hook_id);
        sscanf(line + 100, "%100s", state);
        if (strcmp(state, "enabled") == 0)
        {
            strcpy(hook_list[hooks_count++], hook_id);
        }
    }
    fclose(hooks_file);

    if (hooks_count == 0)
    {
        fprintf(stderr, "No hooks enabled to check");
        return -1;
    }

    DIR *stage = opendir(".kambiz/stage");
    struct dirent *staged_entry;
    while ((staged_entry = readdir(stage)) != NULL)
    {
        if ((staged_entry->d_name[0] == '.') || (staged_entry->d_type != DT_REG))
        {
            continue;
        }

        char staged_file_address[MAX_PATH_LENGTH];
        sprintf(staged_file_address, ".kambiz/stage/%s", staged_entry->d_name);

        FILE *staged_file = fopen(staged_file_address, "r");
        char staged_file_content[MAX_FILE_SIZE] = "";
        char temp;
        while ((temp = fgetc(staged_file)) != EOF)
        {
            strncat(staged_file_content, &temp, 1);
        }
        fclose(staged_file);

        char format[MAX_FORMAT_LENGTH] = "";
        char staged_file_name[MAX_PATH_LENGTH] = "";
        split_by_chars(staged_entry->d_name, staged_file_name, format, ".");

        struct stat filedata;
        stat(staged_file_address, &filedata);

        fprintf(stdout, "%s:\n", staged_entry->d_name);
        for (int i = 0; i < hooks_count; i++)
        {
            fprintf(stdout, "%s: ", hook_list[i]);
            if (strcmp(hook_list[i], "todo-check") == 0)
            {
                if (strcmp(format, "txt") == 0)
                {
                    if (strstr(staged_file_content, "TODO") == NULL)
                    {
                        fprintf(stdout, GREEN "PASSED" COLOR_RESET "\n");
                    }

                    else
                    {
                        fprintf(stdout, RED "FAILED" COLOR_RESET "\n");
                    }
                }

                else if ((strcmp(format, "c") == 0) || (strcmp(format, "cpp") == 0))
                {
                    if (strstr(staged_file_content, "//TODO") == NULL)
                    {
                        fprintf(stdout, GREEN "PASSED" COLOR_RESET "\n");
                    }

                    else
                    {
                        fprintf(stdout, RED "FAILED" COLOR_RESET "\n");
                    }
                }

                else
                {
                    fprintf(stdout, YELLOW "SKIPPED" COLOR_RESET "\n");
                }
            }

            else if (strcmp(hook_list[i], "eof-blank-space") == 0)
            {
                if ((strcmp(format, "c") == 0) || (strcmp(format, "cpp") == 0) || (strcmp(format, "txt") == 0))
                {
                    if ((staged_file_content[strlen(staged_file_content) - 1] != ' ') &&
                        (staged_file_content[strlen(staged_file_content) - 1] != '\t') &&
                        (staged_file_content[strlen(staged_file_content) - 1] != '\n'))
                    {
                        fprintf(stdout, GREEN "PASSED" COLOR_RESET "\n");
                    }

                    else
                    {
                        fprintf(stdout, RED "FAILED" COLOR_RESET "\n");
                    }
                }

                else
                {
                    fprintf(stdout, YELLOW "SKIPPED" COLOR_RESET "\n");
                }
            }

            else if (strcmp(hook_list[i], "format-check") == 0)
            {
                if ((strcmp(format, "c") == 0) || (strcmp(format, "cpp") == 0) || (strcmp(format, "txt") == 0) ||
                    (strcmp(format, "mp4") == 0) || (strcmp(format, "wav") == 0) || (strcmp(format, "mp3") == 0))
                {
                    fprintf(stdout, GREEN "PASSED" COLOR_RESET "\n");
                }

                else
                {
                    fprintf(stdout, RED "FAILED" COLOR_RESET "\n");
                }
            }

            else if (strcmp(hook_list[i], "balance-braces") == 0)
            {
                if ((strcmp(format, "c") == 0) || (strcmp(format, "cpp") == 0) || (strcmp(format, "txt") == 0))
                {
                    if ((char_counter(staged_file_content, '(') == char_counter(staged_file_content, ')')) &&
                        (char_counter(staged_file_content, '[') == char_counter(staged_file_content, ']')) &&
                        (char_counter(staged_file_content, '{') == char_counter(staged_file_content, '}')))
                    {
                        fprintf(stdout, GREEN "PASSED" COLOR_RESET "\n");
                    }

                    else
                    {
                        fprintf(stdout, RED "FAILED" COLOR_RESET "\n");
                    }
                }

                else
                {
                    fprintf(stdout, YELLOW "SKIPPED" COLOR_RESET "\n");
                }
            }

            else if (strcmp(hook_list[i], "file-size-check") == 0)
            {
                if (filedata.st_size <= 5242880)
                {
                    fprintf(stdout, GREEN "PASSED" COLOR_RESET "\n");
                }

                else
                {
                    fprintf(stdout, RED "FAILED" COLOR_RESET "\n");
                }
            }

            else if (strcmp(hook_list[i], "character-limit") == 0)
            {
                if ((strcmp(format, "c") == 0) || (strcmp(format, "cpp") == 0) || (strcmp(format, "txt") == 0))
                {
                    if (strlen(staged_file_content) <= 20000)
                    {
                        fprintf(stdout, GREEN "PASSED" COLOR_RESET "\n");
                    }

                    else
                    {
                        fprintf(stdout, RED "FAILED" COLOR_RESET "\n");
                    }
                }

                else
                {
                    fprintf(stdout, YELLOW "SKIPPED" COLOR_RESET "\n");
                }
            }

            else if (strcmp(hook_list[i], "static-error-check") == 0)
            {
                if ((strcmp(format, "c") == 0) || (strcmp(format, "cpp") == 0))
                {
                    char command[MAX_FULL_COMMAND_LENGTH];
                    sprintf(command, "gcc %s -o test 2>/dev/null", staged_entry->d_name);
                    if (system(command) == 0)
                    {
                        fprintf(stdout, GREEN "PASSED" COLOR_RESET "\n");
                    }

                    else
                    {
                        fprintf(stdout, RED "FAILED" COLOR_RESET "\n");
                    }
                }

                else
                {
                    fprintf(stdout, YELLOW "SKIPPED" COLOR_RESET "\n");
                }
            }
        }
    }
    closedir(stage);
    return 1;
}

int pre_commit_f(char **file_names, int file_count)
{
    FILE *hooks_file = fopen(".kambiz/hooks.txt", "r");
    char line[202];
    char hook_list[10][100];
    int hooks_count = 0;
    while (fgets(line, 202, hooks_file) != NULL)
    {
        char hook_id[101];
        char state[101];
        sscanf(line, "%100s", hook_id);
        sscanf(line + 100, "%100s", state);
        if (strcmp(state, "enabled") == 0)
        {
            strcpy(hook_list[hooks_count++], hook_id);
        }
    }
    fclose(hooks_file);

    if (hooks_count == 0)
    {
        fprintf(stderr, "No hooks enabled to check");
        return -1;
    }

    for (int j = 0; j < file_count; j++)
    {
        char staged_file_address[MAX_PATH_LENGTH];
        sprintf(staged_file_address, ".kambiz/stage/%s", file_names[j]);

        FILE *staged_file = fopen(staged_file_address, "r");
        if (staged_file == NULL)
        {
            fprintf(stdout, "%s doesn't exist in staging area", file_names[j]);
            continue;
        }

        char staged_file_content[MAX_FILE_SIZE] = "";
        char temp;
        while ((temp = fgetc(staged_file)) != EOF)
        {
            strncat(staged_file_content, &temp, 1);
        }
        fclose(staged_file);

        char format[MAX_FORMAT_LENGTH] = "";
        char staged_file_name[MAX_PATH_LENGTH] = "";
        split_by_chars(file_names[j], staged_file_name, format, ".");

        struct stat filedata;
        stat(staged_file_address, &filedata);

        fprintf(stdout, "%s:\n", file_names[j]);
        for (int i = 0; i < hooks_count; i++)
        {
            fprintf(stdout, "%s: ", hook_list[i]);
            if (strcmp(hook_list[i], "todo-check") == 0)
            {
                if (strcmp(format, "txt") == 0)
                {
                    if (strstr(staged_file_content, "TODO") == NULL)
                    {
                        fprintf(stdout, GREEN "PASSED" COLOR_RESET "\n");
                    }

                    else
                    {
                        fprintf(stdout, RED "FAILED" COLOR_RESET "\n");
                    }
                }

                else if ((strcmp(format, "c") == 0) || (strcmp(format, "cpp") == 0))
                {
                    if (strstr(staged_file_content, "//TODO") == NULL)
                    {
                        fprintf(stdout, GREEN "PASSED" COLOR_RESET "\n");
                    }

                    else
                    {
                        fprintf(stdout, RED "FAILED" COLOR_RESET "\n");
                    }
                }

                else
                {
                    fprintf(stdout, YELLOW "SKIPPED" COLOR_RESET "\n");
                }
            }

            else if (strcmp(hook_list[i], "eof-blank-space") == 0)
            {
                if ((strcmp(format, "c") == 0) || (strcmp(format, "cpp") == 0) || (strcmp(format, "txt") == 0))
                {
                    if ((staged_file_content[strlen(staged_file_content) - 1] != ' ') &&
                        (staged_file_content[strlen(staged_file_content) - 1] != '\t') &&
                        (staged_file_content[strlen(staged_file_content) - 1] != '\n'))
                    {
                        fprintf(stdout, GREEN "PASSED" COLOR_RESET "\n");
                    }

                    else
                    {
                        fprintf(stdout, RED "FAILED" COLOR_RESET "\n");
                    }
                }

                else
                {
                    fprintf(stdout, YELLOW "SKIPPED" COLOR_RESET "\n");
                }
            }

            else if (strcmp(hook_list[i], "format-check") == 0)
            {
                if ((strcmp(format, "c") == 0) || (strcmp(format, "cpp") == 0) || (strcmp(format, "txt") == 0) ||
                    (strcmp(format, "mp4") == 0) || (strcmp(format, "wav") == 0) || (strcmp(format, "mp3") == 0))
                {
                    fprintf(stdout, GREEN "PASSED" COLOR_RESET "\n");
                }

                else
                {
                    fprintf(stdout, RED "FAILED" COLOR_RESET "\n");
                }
            }

            else if (strcmp(hook_list[i], "balance-braces") == 0)
            {
                if ((strcmp(format, "c") == 0) || (strcmp(format, "cpp") == 0) || (strcmp(format, "txt") == 0))
                {
                    if ((char_counter(staged_file_content, '(') == char_counter(staged_file_content, ')')) &&
                        (char_counter(staged_file_content, '[') == char_counter(staged_file_content, ']')) &&
                        (char_counter(staged_file_content, '{') == char_counter(staged_file_content, '}')))
                    {
                        fprintf(stdout, GREEN "PASSED" COLOR_RESET "\n");
                    }

                    else
                    {
                        fprintf(stdout, RED "FAILED" COLOR_RESET "\n");
                    }
                }

                else
                {
                    fprintf(stdout, YELLOW "SKIPPED" COLOR_RESET "\n");
                }
            }

            else if (strcmp(hook_list[i], "file-size-check") == 0)
            {
                if (filedata.st_size <= 5242880)
                {
                    fprintf(stdout, GREEN "PASSED" COLOR_RESET "\n");
                }

                else
                {
                    fprintf(stdout, RED "FAILED" COLOR_RESET "\n");
                }
            }

            else if (strcmp(hook_list[i], "character-limit") == 0)
            {
                if ((strcmp(format, "c") == 0) || (strcmp(format, "cpp") == 0) || (strcmp(format, "txt") == 0))
                {
                    if (strlen(staged_file_content) <= 20000)
                    {
                        fprintf(stdout, GREEN "PASSED" COLOR_RESET "\n");
                    }

                    else
                    {
                        fprintf(stdout, RED "FAILED" COLOR_RESET "\n");
                    }
                }

                else
                {
                    fprintf(stdout, YELLOW "SKIPPED" COLOR_RESET "\n");
                }
            }

            else if (strcmp(hook_list[i], "static-error-check") == 0)
            {
                if ((strcmp(format, "c") == 0) || (strcmp(format, "cpp") == 0))
                {
                    char command[MAX_FULL_COMMAND_LENGTH];
                    sprintf(command, "gcc %s -o test 2>/dev/null", file_names[j]);
                    if (system(command) == 0)
                    {
                        fprintf(stdout, GREEN "PASSED" COLOR_RESET "\n");
                    }

                    else
                    {
                        fprintf(stdout, RED "FAILED" COLOR_RESET "\n");
                    }
                }

                else
                {
                    fprintf(stdout, YELLOW "SKIPPED" COLOR_RESET "\n");
                }
            }
        }
    }
    return 1;
}

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        fprintf(stderr, "Please enter a command");
        return 0;
    }

    // Process Addresses
    getcwd(local_address, MAX_PATH_LENGTH);
    char global_aliases_address[MAX_PATH_LENGTH] = "/Users/sobhan/Documents/Kambiz/aliases.txt";
    char local_aliases_address[MAX_PATH_LENGTH] = "";
    sprintf(local_aliases_address, "%s/.kambiz/aliases.txt", local_address);

    // Check Init
    DIR *working_tree = opendir(local_address);
    if (search_in_directory(working_tree, ".kambiz", DT_DIR))
    {
        init_done = true;
    }
    closedir(working_tree);

    // Update User Data
    update_user_data();

    if (init_done)
    {
        // Get Branch Name
        update_branch_name();
        update_id();

        // Get Last ID
        update_last_commit_id();

        // Check being on head
        on_head = (find_branch_head_n_id(current_branch_name, 1) == current_id);
    }

    // Check Aliases
    char alias_value[MAX_FULL_COMMAND_LENGTH] = "";
    FILE *global_aliases = fopen(global_aliases_address, "r");
    get_value_by_attribute(global_aliases, argv[1], alias_value, 1, 1, true);
    fclose(global_aliases);
    if ((strcmp(alias_value, "") == 0) && (init_done))
    {
        FILE *local_aliases = fopen(local_aliases_address, "r");
        get_value_by_attribute(local_aliases, argv[1], alias_value, 1, 1, true);
        fclose(local_aliases);
    }
    if (strcmp(alias_value, "") != 0)
    {
        int alias_value_word_count = 0;
        char alias_value_words[10][MAX_STRING_LENGTH];

        char *word = strtok(alias_value, " \n");
        while (word != NULL)
        {
            strcpy(alias_value_words[alias_value_word_count++], word);
            word = strtok(NULL, " \n");
        }

        argc = alias_value_word_count;
        argv = (char **)malloc(argc * sizeof(char *));
        for (int i = 0; i < argc; i++)
        {
            argv[i] = (char *)malloc(strlen(alias_value_words[i]) + 1);
            strcpy(argv[i], alias_value_words[i]);
        }
    }

    if ((strcmp(argv[1], "config") == 0))
    {
        if ((argc == 5) && (strcmp(argv[2], "-global") == 0))
        {
            config(argv[3], argv[4], true);
            return 0;
        }
        if (argc == 4)
        {
            config(argv[2], argv[3], false);
            return 0;
        }
    }

    if ((strcmp(argv[1], "init") == 0) && (argc == 2))
    {
        init();
        return 0;
    }

    if ((strcmp(argv[1], "add") == 0) && (argc > 2))
    {
        if (!init_done)
        {
            fprintf(stderr, "Initializtion must be done first");
            return 0;
        }
        if (strcmp(argv[2], "-redo") == 0)
        {
            add_redo();
            return 0;
        }
        if ((strcmp(argv[2], "-n") == 0) && (argc == 4))
        {
            char output[10000] = "";
            int depth;
            sscanf(argv[3], "%d", &depth);
            add_n(".", 1, depth, output);
            fprintf(stdout, "%s", output);
            return 0;
        }
        if (strcmp(argv[2], "-f") == 0)
        {
            add(argv + 3, argc - 3);
            return 0;
        }
        else
        {
            add(argv + 2, argc - 2);
            return 0;
        }
    }

    if ((strcmp(argv[1], "reset") == 0) && (argc > 2))
    {
        if (!init_done)
        {
            fprintf(stderr, "Initializtion must be done first");
            return 0;
        }
        if (strcmp(argv[2], "-undo") == 0)
        {
            reset_undo();
            return 0;
        }
        if (strcmp(argv[2], "-f") == 0)
        {
            reset(argv + 3, argc - 3);
            return 0;
        }
        else
        {
            reset(argv + 2, argc - 2);
            return 0;
        }
    }

    if ((strcmp(argv[1], "status") == 0) && (argc == 2))
    {
        status();
        return 0;
    }

    if (strcmp(argv[1], "commit") == 0)
    {
        if (argc == 3)
        {
            fprintf(stderr, "Please enter a message");
            return -1;
        }
        else if ((argc == 4) && (strcmp(argv[2], "-m") == 0))
        {
            commit(argv[3]);
            return 0;
        }
        else if ((argc == 4) && (strcmp(argv[2], "-s") == 0))
        {
            char message[MAX_STRING_LENGTH];
            char shortcut_file_address[MAX_PATH_LENGTH];
            sprintf(shortcut_file_address, ".kambiz/shortcuts/%s.txt", argv[3]);
            FILE *shortcut_file = fopen(shortcut_file_address, "r");
            if (shortcut_file == NULL)
            {
                fprintf(stderr, "No shortcut exists with this name");
                return -1;
            }
            fscanf(shortcut_file, "%[^\n]s", message);
            fclose(shortcut_file);
            commit(message);
            return 0;
        }
    }

    if ((strcmp(argv[1], "set") == 0) && (argc == 6))
    {
        set_shortcut(argv[5], argv[3]);
        return 0;
    }

    if ((strcmp(argv[1], "replace") == 0) && (argc == 6))
    {
        replace_shortcut(argv[5], argv[3]);
        return 0;
    }

    if ((strcmp(argv[1], "remove") == 0) && (argc == 4))
    {
        remove_shortcut(argv[3]);
        return 0;
    }

    if (strcmp(argv[1], "branch") == 0)
    {
        if (argc == 2)
        {
            show_branches();
            return 0;
        }
        if (argc == 3)
        {
            branch(argv[2]);
            return 0;
        }
    }

    if ((strcmp(argv[1], "checkout") == 0) && (argc == 3))
    {
        char before_dash[MAX_SHORT_COMMAND_LENGTH] = "";
        char after_dash[MAX_SHORT_COMMAND_LENGTH] = "";
        split_by_chars(argv[2], before_dash, after_dash, "-");

        if (strcmp(before_dash, "HEAD") == 0)
        {
            int n = 0;
            if (strcmp(after_dash, "") != 0)
            {
                sscanf(after_dash, "%d", &n);
            }
            checkout_commit(current_branch_name, find_branch_head_n_id(current_branch_name, n + 1));
            return 0;
        }

        DIR *branches = opendir(".kambiz/branches");
        if (search_in_directory(branches, argv[2], DT_DIR) != NULL)
        {
            closedir(branches);
            checkout_commit(argv[2], find_branch_head_n_id(argv[2], 1));
            return 0;
        }

        char commit_branch[MAX_STRING_LENGTH] = "";
        FILE *log_file = fopen(".kambiz/branches/commit_log.txt", "r");
        get_value_by_attribute(log_file, argv[2], commit_branch, 5, 1, false);
        if (strcmp(commit_branch, "") != 0)
        {
            int id;
            sscanf(argv[2], "%d", &id);
            checkout_commit(commit_branch, id);
        }
        return 0;
    }

    if (strcmp(argv[1], "log") == 0)
    {
        if (argc == 2)
        {
            log_filter("", argv + 3, 0);
            return 0;
        }

        if (argc == 4)
        {
            log_filter(argv[2], argv + 3, 1);
            return 0;
        }

        if (strcmp(argv[1], "-search") == 0)
        {
            log_filter(argv[2], argv + 3, argc - 3);
            return 0;
        }
    }

    if ((strcmp(argv[1], "revert") == 0) && (argc >= 3))
    {
        char before_dash[MAX_SHORT_COMMAND_LENGTH] = "";
        char after_dash[MAX_SHORT_COMMAND_LENGTH] = "";
        split_by_chars(argv[argc - 1], before_dash, after_dash, "-");

        if (strcmp(before_dash, "HEAD") == 0)
        {
            int n = 0;
            if (strcmp(after_dash, "") != 0)
            {
                sscanf(after_dash, "%d", &n);
            }

            if (argc == 3)
            {
                char last_message[MAX_STRING_LENGTH] = "";
                FILE *log_file = fopen(".kambiz/branches/commit_log.txt", "r");
                char id_string[MAX_STRING_LENGTH] = "";
                sprintf(id_string, "%d", find_branch_head_n_id(current_branch_name, n));
                get_value_by_attribute(log_file, id_string, last_message, 5, 5, true);
                fclose(log_file);
                revert(current_branch_name, find_branch_head_n_id(current_branch_name, n), last_message);
                return 0;
            }

            else if ((argc == 5) && (strcmp(argv[2], "-m") == 0))
            {
                revert(current_branch_name, find_branch_head_n_id(current_branch_name, n), argv[3]);
                return 0;
            }
        }

        char commit_branch[MAX_STRING_LENGTH] = "";
        int id = 0;
        char last_message[MAX_STRING_LENGTH] = "";
        FILE *log_file = fopen(".kambiz/branches/commit_log.txt", "r");
        get_value_by_attribute(log_file, argv[argc - 1], commit_branch, 5, 1, false);
        get_value_by_attribute(log_file, argv[argc - 1], last_message, 5, 5, true);
        fclose(log_file);
        if (strcmp(commit_branch, "") != 0)
        {
            sscanf(argv[argc - 1], "%d", &id);
        }

        if (id != 0)
        {
            if ((strcmp(argv[2], "-n") == 0) && (argc == 4))
            {
                checkout_commit(commit_branch, id);
                return 0;
            }

            else if (argc == 3)
            {
                revert(commit_branch, id, last_message);
                return 0;
            }

            else if ((argc == 5) && (strcmp(argv[2], "-m") == 0))
            {
                revert(commit_branch, id, argv[3]);
                return 0;
            }
        }
    }

    if ((strcmp(argv[1], "tag") == 0))
    {
        if (argc == 2)
        {
            list_tags();
            return 0;
        }
        if ((strcmp(argv[2], "-a") == 0) && (argc >= 4))
        {
            bool dash_f = false;
            if (strcmp(argv[argc - 1], "-f") == 0)
            {
                dash_f = true;
            }

            char message[MAX_STRING_LENGTH] = "";
            if (argc > 4)
            {
                if (strcmp(argv[4], "-m") == 0)
                {
                    strcpy(message, argv[5]);
                }
            }

            char branch_name[MAX_STRING_LENGTH];
            char id_string[MAX_STRING_LENGTH];
            if (strcmp(argv[argc - 2], "-c") == 0)
            {
                strcpy(id_string, argv[argc - 1]);
                FILE *log_file = fopen(".kambiz/branches/commit_log.txt", "r");
                get_value_by_attribute(log_file, id_string, branch_name, 5, 1, false);
                fclose(log_file);
            }
            else if (strcmp(argv[argc - 3], "-c") == 0)
            {
                strcpy(id_string, argv[argc - 2]);
                FILE *log_file = fopen(".kambiz/branches/commit_log.txt", "r");
                get_value_by_attribute(log_file, id_string, branch_name, 5, 1, false);
                fclose(log_file);
            }
            else
            {
                strcpy(branch_name, current_branch_name);
                sprintf(id_string, "%d", find_branch_head_n_id(current_branch_name, 1));
            }

            add_tag(argv[3], id_string, branch_name, message, dash_f);
            return 0;
        }

        if ((strcmp(argv[2], "show") == 0) && (argc == 4))
        {
            show_tag(argv[3]);
            return 0;
        }
    }

    if (strcmp(argv[1], "diff") == 0)
    {
        if ((strcmp(argv[2], "-f") == 0) && (argc >= 5))
        {
            int begin1 = 1;
            int begin2 = 1;
            int end1 = MAX_INT_VALUE;
            int end2 = MAX_INT_VALUE;
            if (argc > 5)
            {
                if (strcmp(argv[5], "-line1") == 0)
                {
                    sscanf(argv[6], "%d-%d", &begin1, &end1);
                }
                if (strcmp(argv[argc - 2], "-line2") == 0)
                {
                    sscanf(argv[argc - 1], "%d-%d", &begin2, &end2);
                }
            }

            file_diff(argv[3], argv[4], begin1, end1, begin2, end2);
            return 0;
        }

        if ((strcmp(argv[2], "-c") == 0) && (argc == 5))
        {
            char commit_branch1[MAX_STRING_LENGTH] = "";
            char commit_branch2[MAX_STRING_LENGTH] = "";
            FILE *log_file = fopen(".kambiz/branches/commit_log.txt", "r");
            get_value_by_attribute(log_file, argv[3], commit_branch1, 5, 1, false);
            get_value_by_attribute(log_file, argv[4], commit_branch2, 5, 1, false);
            fclose(log_file);
            commit_diff(commit_branch1, argv[3], commit_branch2, argv[4]);
            return 0;
        }
    }

    if (strcmp(argv[1], "pre-commit") == 0)
    {
        if (argc == 2)
        {
            pre_commit();
            return 0;
        }
        if (strcmp(argv[2], "-f") == 0)
        {
            pre_commit_f(argv + 2, argc - 2);
            return 0;
        }
        if ((strcmp(argv[2], "hooks") == 0) && (strcmp(argv[3], "list") == 0))
        {
            list_hooks();
            return 0;
        }
        if ((strcmp(argv[2], "applied") == 0) && (strcmp(argv[3], "hooks") == 0))
        {
            list_applied_hooks();
            return 0;
        }
        if ((strcmp(argv[2], "add") == 0) && (strcmp(argv[3], "hook") == 0))
        {
            add_hook(argv[4]);
            return 0;
        }
        if ((strcmp(argv[2], "remove") == 0) && (strcmp(argv[3], "hook") == 0))
        {
            remove_hook(argv[4]);
            return 0;
        }
    }

    if ((strcmp(argv[1], "grep") == 0) && (argc >= 6))
    {
        char commit_branch[MAX_STRING_LENGTH];
        char id_string[MAX_STRING_LENGTH];

        if (argc >= 8)
        {
            if (strcmp(argv[6], "-c") == 0)
            {
                FILE *log_file = fopen(".kambiz/branches/commit_log.txt", "r");
                get_value_by_attribute(log_file, argv[7], commit_branch, 5, 1, false);
                strcpy(id_string, argv[7]);
                fclose(log_file);
            }
        }

        else
        {
            strcpy(commit_branch, current_branch_name);
            sprintf(id_string, "%d", current_id);
        }

        bool dash_n = false;
        if (strcmp(argv[argc - 1], "-n") == 0)
        {
            dash_n = true;
        }

        grep(argv[3], argv[5], id_string, commit_branch, dash_n);
        return 0;
    }

    fprintf(stderr, "Invalid Command");
    return 0;
}

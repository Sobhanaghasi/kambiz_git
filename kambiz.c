// To Be Done:
// Alias Check
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
#define MAX_FULL_COMMAND_LENGTH 1000
#define MAX_WORD_COUNT 50
#define MAX_SHORT_COMMAND_LENGTH 50

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

void change_value_by_attribute(FILE *file, char attribute[], char value[], int value_count, int wanted_value)
{
    char line[MAX_STRING_LENGTH];
    long line_position = -1;
    while (fscanf(file, "%[^\n]s", line) != EOF)
    {
        char line_attribute[MAX_STRING_LENGTH] = "";
        sscanf(line, "%100s", line_attribute);
        if (strcmp(line_attribute, attribute) == 0)
        {
            line_position = ftell(file) - (100 * value_count) - 100;
            break;
        }
        fscanf(file, "\n");
    }
    if (line_position == -1)
    {
        return;
    }

    fseek(file, line_position + (wanted_value * 100), SEEK_SET);
    fprintf(file, "%-100s", value);
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

    char add_log_address[MAX_PATH_LENGTH] = ".kambiz/add_log.txt";
    FILE *add_log_file = fopen(add_log_address, "w");
    fclose(add_log_file);

    char deleting_stage_address[MAX_PATH_LENGTH] = ".kambiz/deleting_stage.txt";
    FILE *deleting_stage_file = fopen(deleting_stage_address, "w");
    fclose(deleting_stage_file);

    mkdir(".kambiz/stage", 0777);
    mkdir(".kambiz/branches", 0777);
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
                change_value_by_attribute(config_file, attribute_value, value, 1, 1);
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
                change_value_by_attribute(config_file, attribute_value, value, 1, 1);
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
            change_value_by_attribute(config_file, attribute_value, value, 1, 1);
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
    printf("Boro dirooz bia");
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
    /*char alias_value[MAX_FULL_COMMAND_LENGTH] = "";
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
    }*/

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
        else if (argc == 4)
        {
            commit(argv[3]);
            return 0;
        }
    }

    if (strcmp(argv[1], "branch") == 0)
    {
        if (argc == 2)
        {
            // show_branches();
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
        char commit_branch[MAX_STRING_LENGTH] = "";
        int id = 0;
        char last_message[MAX_STRING_LENGTH] = "";
        FILE *log_file = fopen(".kambiz/branches/commit_log.txt", "r");
        get_value_by_attribute(log_file, argv[argc - 1], commit_branch, 5, 1, false);
        get_value_by_attribute(log_file, argv[argc - 1], last_message, 5, 5, true);
        if (strcmp(commit_branch, "") != 0)
        {
            sscanf(argv[argc - 1], "%d", &id);
        }

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
                revert(current_branch_name, find_branch_head_n_id(current_branch_name, n), last_message);
                return 0;
            }

            else if ((argc == 5) && (strcmp(argv[2], "-m") == 0))
            {
                revert(current_branch_name, find_branch_head_n_id(current_branch_name, n), argv[3]);
                return 0;
            }
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
    fprintf(stderr, "Invalid Command");
    return 0;
}

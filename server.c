#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<pthread.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define RESPONSE_SIZE 8192
#define FILENAME "users.txt"
int client_counter = 0;           // Global client counter
pthread_mutex_t counter_mutex;    // Mutex for thread-safe counter access
char connected_users[100][50];    // Store connected usernames
int connected_count = 0;          // Track number of connected users
int client_ids[100];              // Store associated client IDs


/*------------------------------------------------------------------- FUNCTII PENTRU LOGAREA CLIENTILOR ------------------------------------------------------------------------------------------------*/
// Functie pentru a verifica daca un nume de utilizator este deja conectat
int is_user_connected(const char *username) 
{
    for (int i = 0; i < connected_count; i++) 
    {
        if (strcmp(connected_users[i], username) == 0) 
        {
            return 1; // User already connected
        }
    }
    return 0;
}

// Functie pentru verificarea utilizatorului in fisier
int authenticate_user(const char *username, const char *password) 
{
    FILE *file = fopen(FILENAME, "r");
    if (file == NULL) 
    {
        perror("Nu s-a putut deschide fisierul");
        return 0;
    }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) 
    {
        // Eliminam newline-ul de la sfarsit, daca exista
        line[strcspn(line, "\n")] = 0;

        // Comparam username si password cu linia din fisier
        if (strcmp(line, username) == 0 && strcmp(line, password) == 0) 
        {
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}

// Functie pentru creare director unic pentru fiecare client
void create_client_directory( char *client_dir,char *username) 
{
    snprintf(client_dir, BUFFER_SIZE, "client_%s", username);
    struct stat st = {0};
    if (stat(client_dir, &st) == -1) 
    {
        mkdir(client_dir, 0777);
    }
}





/*----------------------------------------------------------------0. FUNCTIA PENTRU DECONECTAREA CLIENTILOR ------------------------------------------------------------------------------------------------*/
// Functie pentru deconectarea unui client
void disconnect_user(int client_id) 
{
    pthread_mutex_lock(&counter_mutex);
    for (int i = 0; i < connected_count; i++) 
    {
        if (client_ids[i] == client_id) 
        {
            for (int j = i; j < connected_count - 1; j++) 
            {
                strcpy(connected_users[j], connected_users[j + 1]);
                client_ids[j] = client_ids[j + 1];
            }
            connected_count--;
            break;
        }
    }
    pthread_mutex_unlock(&counter_mutex);
}





/*---------------------------------------------------------------- 1. FUNCTIA PENTRU AFISAREA ARBORELUI DE FISIERE ------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------- 2. FUNCTIA PENTRU AFISAREA ARBORELUI DE FISIERE PENTRU CLIENT------------------------------------------------------------------------------------------------*/

// Functie pentru vizualizarea arborelui de fisiere intr-un format structurat
const char* traverse_directory(const char *directory_name, int level) 
{
    static char full_response[RESPONSE_SIZE] = {0};
    char temp_buffer[BUFFER_SIZE] = {0};
    DIR *dir;
    struct dirent *entry;
    struct stat file_info;

    if (level == 0) 
    {
        memset(full_response, 0, sizeof(full_response));
    }

    dir = opendir(directory_name);
    if (!dir) 
    {
        snprintf(temp_buffer, BUFFER_SIZE, "Error opening directory: %s\n", directory_name);
        strcat(full_response, temp_buffer);
        return full_response;
    }

    for (int i = 0; i < level; i++) 
    {
        strcat(full_response, "  ");
    }
    snprintf(temp_buffer, BUFFER_SIZE, "|_ %s\n", directory_name);
    strcat(full_response, temp_buffer);

    while ((entry = readdir(dir)) != NULL) 
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) 
        {
            char file_path[1000];
            snprintf(file_path, sizeof(file_path), "%s/%s", directory_name, entry->d_name);

            if (stat(file_path, &file_info) < 0) 
            {
                snprintf(temp_buffer, BUFFER_SIZE, "Error reading info for: %s\n", file_path);
                strcat(full_response, temp_buffer);
            } 
            else 
            {
                if (S_ISDIR(file_info.st_mode)) 
                {
                    traverse_directory(file_path, level + 1);
                } 
                else 
                {
                    for (int i = 0; i < level + 1; i++) 
                    {
                        strcat(full_response, "  ");
                    }
                    snprintf(temp_buffer, BUFFER_SIZE, "|_ %s (File)\n", entry->d_name);
                    strcat(full_response, temp_buffer);
                }
            }
        }
    }

    closedir(dir);
    return full_response;
}





/*------------------------------------------------------------------- 3. FUNCTII PENTRU TRANSFERUL FISIERELOR ------------------------------------------------------------------------------------------------*/
// Functie pentru copierea unui fisier
int copy_file(const char *source_path, const char *destination_path) 
{
    FILE *source = fopen(source_path, "rb");
    if (!source) 
    {
        perror("Error opening source file");
        return -1;
    }

    FILE *destination = fopen(destination_path, "wb");
    if (!destination) 
    {
        perror("Error opening destination file");
        fclose(source);
        return -1;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes;
    while ((bytes = fread(buffer, 1, BUFFER_SIZE, source)) > 0) 
    {
        fwrite(buffer, 1, bytes, destination);
    }

    fclose(source);
    fclose(destination);
    return 0;
}


void incarcare_fisier(const char *source_path, const char *destination_path, int client_socket) 
{
    char response[BUFFER_SIZE];
    if (copy_file(source_path, destination_path) == 0) 
    {
        snprintf(response, sizeof(response), "Fisierul '%s' a fost incarcat cu succes in '%s'.\n", source_path, destination_path);
    } 
    else 
    {
        snprintf(response, sizeof(response), "Eroare: Nu s-a putut incarca fisierul '%s' in '%s'.\n", source_path, destination_path);
    }
    send(client_socket, response, strlen(response), 0);
}





/*---------------------------------------------------------------- 4. FUNCTII PENTRU STERGEREA FISIERELOR/DIRECTOARELOR ------------------------------------------------------------------------------------------------*/
// Function to delete a directory and its contents
int delete_directory(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("Error opening directory");
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char entry_path[BUFFER_SIZE];
        snprintf(entry_path, sizeof(entry_path), "%s/%s", path, entry->d_name);

        struct stat statbuf;
        if (stat(entry_path, &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                // Recursive delete for directories
                if (delete_directory(entry_path) < 0) {
                    closedir(dir);
                    return -1;
                }
            } else {
                // Delete file
                if (remove(entry_path) < 0) {
                    perror("Error deleting file");
                    closedir(dir);
                    return -1;
                }
            }
        }
    }

    closedir(dir);

    // Remove the empty directory
    if (rmdir(path) < 0) {
        perror("Error removing directory");
        return -1;
    }

    return 0;
}


// Function to search for and delete a file or directory
int search_and_delete(const char *directory, const char *filename) 
{
    DIR *dir = opendir(directory);
    if (!dir) 
    {
        perror("Error opening directory");
        return 0;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) 
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) 
        {
            continue;
        }

        char path[BUFFER_SIZE];
        snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

        struct stat statbuf;
        if (stat(path, &statbuf) == 0) 
        {
            if (S_ISDIR(statbuf.st_mode)) 
            {
                if (strcmp(entry->d_name, filename) == 0) 
                {
                    // Delete directory
                    if (delete_directory(path) == 0) 
                    {
                        closedir(dir);
                        return 1;
                    }
                } 
                else 
                {
                    // Recursive search in subdirectories
                    if (search_and_delete(path, filename)) 
                    {
                        closedir(dir);
                        return 1;
                    }
                }
            } 
            else if (strcmp(entry->d_name, filename) == 0) 
            {
                // Delete file
                if (remove(path) == 0) 
                {
                    closedir(dir);
                    return 1;
                }
            }
        }
    }

    closedir(dir);
    return 0; // File or directory not found
}


void sterge_fisier(const char *file_name, int client_socket) 
{
    char response[BUFFER_SIZE];
    if (search_and_delete(".", file_name)) 
    {
        snprintf(response, sizeof(response), "Fisierul sau direcorul '%s' a fost sters cu succes.\n", file_name);
    } 
    else 
    {
        snprintf(response, sizeof(response), "Eroare: Fisierul sau directorul '%s' nu a fost gasit.\n", file_name);
    }
    send(client_socket, response, strlen(response), 0);
}





/*---------------------------------------------------------------- 5. FUNCTII PENTRU CAUTAREA FISIERELOR/DIRECTOARELOR ------------------------------------------------------------------------------------------------*/
// Functie recursiva pentru a cauta un fisier intr-un director
int search_in_directory(const char *directory, const char *filename, char *result) 
{
    DIR *dir = opendir(directory);
    if (!dir) 
    {
        perror("Eroare la deschiderea directorului");
        return 0;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) 
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) 
        {
            continue;
        }

        char path[BUFFER_SIZE];
        snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

        struct stat statbuf;
        if (stat(path, &statbuf) == 0) 
        {
            if (S_ISDIR(statbuf.st_mode)) 
            {
                // Cautare recursiva in subdirectoare
                if (search_in_directory(path, filename, result)) 
                {
                    closedir(dir);
                    return 1;
                }
            } 
            else if (strcmp(entry->d_name, filename) == 0) 
            {
                realpath(path, result);
                closedir(dir);
                return 1;
            }
        }
    }

    closedir(dir);
    return 0;
}

// Functie pentru gestionarea cererii de cautare
void handle_search(int client_socket) 
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    // Citim cererea de cautare
    read(client_socket, buffer, BUFFER_SIZE);
    char *search_query = buffer + 7; // Sarim peste "SEARCH:" prefix

    char result[BUFFER_SIZE] = "NOT_FOUND";
    if (search_in_directory(".", search_query, result)) 
    {
        send(client_socket, result, strlen(result), 0);
    } 
    else 
    {
        send(client_socket, result, strlen(result), 0);
    }
}





/*---------------------------------------------------------------- 6. FUNCTII PENTRU REALIZAREA SNAPSHOTULUI ------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------6.1 SNAPSHOT IN DIRECTORUL DE OUTPUT DE PE SERVER----------------------------------------------------------------------------------------- */
void create_snapshot(const char *directory_name, const char *output_directory, int client_socket, int client_number) 
{
    struct stat st = {0};
    if (stat(output_directory, &st) == -1) 
    {
        mkdir(output_directory, 0777);
    }

    char snapshot_path[1024];
    snprintf(snapshot_path, sizeof(snapshot_path), "%s/snapshot_client_%d.txt", output_directory, client_number);

    int newfile = open(snapshot_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (newfile < 0) 
    {
        char error_msg[] = "Eroare la deschiderea fisierului de snapshot.\n";
        send(client_socket, error_msg, strlen(error_msg), 0);
        return;
    }

    DIR *dir = opendir(directory_name);
    if (!dir) 
    {
        char error_msg[] = "Eroare la accesarea directorului.\n";
        send(client_socket, error_msg, strlen(error_msg), 0);
        close(newfile);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) 
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) 
        {
            char file_path[1000];
            snprintf(file_path, sizeof(file_path), "%s/%s", directory_name, entry->d_name);
            struct stat file_info;
            if (lstat(file_path, &file_info) < 0) 
            {
                printf("Eroare la citirea informatiei despre fisier: %s\n", file_path);
                continue;
            }

            char file_type[100];
            if (S_ISREG(file_info.st_mode)) 
            {
                strcpy(file_type, "Regular file");
            } 
            else if (S_ISDIR(file_info.st_mode)) 
            {
                strcpy(file_type, "Directory");
            } 
            else 
            {
                strcpy(file_type, "Other");
            }

            char permissions[10];
            snprintf(permissions, sizeof(permissions), "%c%c%c%c%c%c%c%c%c",
                     (file_info.st_mode & S_IRUSR) ? 'r' : '-',
                     (file_info.st_mode & S_IWUSR) ? 'w' : '-',
                     (file_info.st_mode & S_IXUSR) ? 'x' : '-',
                     (file_info.st_mode & S_IRGRP) ? 'r' : '-',
                     (file_info.st_mode & S_IWGRP) ? 'w' : '-',
                     (file_info.st_mode & S_IXGRP) ? 'x' : '-',
                     (file_info.st_mode & S_IROTH) ? 'r' : '-',
                     (file_info.st_mode & S_IWOTH) ? 'w' : '-',
                     (file_info.st_mode & S_IXOTH) ? 'x' : '-');

            char buffer[1024];
            snprintf(buffer, sizeof(buffer), "File Name: %s\nFile Type: %s\nPermissions: %s\n\n",
                     entry->d_name, file_type, permissions);
            write(newfile, buffer, strlen(buffer));
        }
    }

    closedir(dir);
    close(newfile);
    /*char success_msg[2048];
    snprintf(success_msg, sizeof(success_msg), "Snapshot creat cu succes: %.1000s\n", snapshot_path);
    send(client_socket, success_msg, strlen(success_msg),0);*/
}
/*-----------------------------------------------------------------6.2 SNAPSHOT PENTRU FIECARE CLIENT IN DIRECTORUL CLIENTULUI----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------6.2.1 CREARE FISIER PENTRU RETINEREA NUMARULUI DE SNAPSHOTURI ALE FIECARUI CLIENT------------------------------------------------------------*/
int get_and_increment_snapshot_counter(const char *client_directory) {
    char counter_path[BUFFER_SIZE];
    snprintf(counter_path, sizeof(counter_path), "%s/snapshot_counter.txt", client_directory);

    // Citim contorul curent sau il initializam daca fisierul nu exista
    FILE *counter_file = fopen(counter_path, "r");
    int counter = 0;

    if (counter_file) {
        fscanf(counter_file, "%d", &counter); // Citim valoarea curenta
        fclose(counter_file);
    }

    printf("Contor inainte de incrementare: %d\n", counter);

    // Scriem noua valoare incrementata in fisier
    counter_file = fopen(counter_path, "w");
    if (counter_file) {
        fprintf(counter_file, "%d", counter + 1); // Incrementam doar pentru salvare
        fclose(counter_file);
    } else {
        perror("Eroare la scrierea contorului de snapshot-uri");
    }

    printf("Contor dupa incrementare: %d\n", counter + 1);

    return counter + 1; // Returnam valoarea dupa incrementare
}
/*------------------------------------------------------------------6.2.2 CREAREA DIRECTORULUI DE SNAPSHOTURI PENTRU FIECARE CLIENT-------------------------------------------------------------------------------*/
void create_snapshots_directory(const char *base_client_directory, char *snap_dir) {
    snprintf(snap_dir, BUFFER_SIZE, "%s/snapshots", base_client_directory);
    struct stat st = {0};
    if (stat(snap_dir, &st) == -1) {
        mkdir(snap_dir, 0777);
    }
}

/*------------------------------------------------------------------6.2.3 CREEARE SNAPSHOTURILOR--------------------------------------------------------------------------------------------------------------------*/
void create_snapshot_for_client(const char *client_directory, const char *snapshot_directory, int client_socket) {
int snapshot_counter = get_and_increment_snapshot_counter(client_directory);
    char snapshot_path[BUFFER_SIZE];

    // Cream numele fisierului snapshot
    snprintf(snapshot_path, sizeof(snapshot_path), "%s/snapshot_%d.txt", snapshot_directory, snapshot_counter);
    snapshot_counter++;

    // Cream fisierul pentru snapshot
    int newfile = open(snapshot_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (newfile < 0) {
        char error_msg[] = "Eroare la deschiderea fisierului de snapshot.\n";
        send(client_socket, error_msg, strlen(error_msg), 0);
        return;
    }

    DIR *dir = opendir(client_directory);
    if (!dir) {
        char error_msg[] = "Eroare la accesarea directorului pentru snapshot.\n";
        send(client_socket, error_msg, strlen(error_msg), 0);
        close(newfile);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char file_path[BUFFER_SIZE];
            snprintf(file_path, sizeof(file_path), "%s/%s", client_directory, entry->d_name);

            struct stat file_info;
            if (lstat(file_path, &file_info) < 0) {
                printf("Eroare la citirea informatiilor despre fisier: %s\n", file_path);
                continue;
            }

            char file_type[100];
            if (S_ISREG(file_info.st_mode)) {
                strcpy(file_type, "Regular file");
            } else if (S_ISDIR(file_info.st_mode)) {
                strcpy(file_type, "Directory");
            } else {
                strcpy(file_type, "Other");
            }

            char permissions[10];
            snprintf(permissions, sizeof(permissions), "%c%c%c%c%c%c%c%c%c",
                     (file_info.st_mode & S_IRUSR) ? 'r' : '-',
                     (file_info.st_mode & S_IWUSR) ? 'w' : '-',
                     (file_info.st_mode & S_IXUSR) ? 'x' : '-',
                     (file_info.st_mode & S_IRGRP) ? 'r' : '-',
                     (file_info.st_mode & S_IWGRP) ? 'w' : '-',
                     (file_info.st_mode & S_IXGRP) ? 'x' : '-',
                     (file_info.st_mode & S_IROTH) ? 'r' : '-',
                     (file_info.st_mode & S_IWOTH) ? 'w' : '-',
                     (file_info.st_mode & S_IXOTH) ? 'x' : '-');

            char buffer[BUFFER_SIZE];
            snprintf(buffer, sizeof(buffer), "File Name: %s\nFile Type: %s\nPermissions: %s\n\n",
                     entry->d_name, file_type, permissions);
            write(newfile, buffer, strlen(buffer));
        }
    }

    closedir(dir);
    close(newfile);

    /*char success_msg[BUFFER_SIZE];
    snprintf(success_msg, sizeof(success_msg), "Snapshot creat cu succes: %.1000s\n", snapshot_path);
    send(client_socket, success_msg, strlen(success_msg), 0);*/
}

/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/




void *handle_client(void *arg) 
{
    int client_socket = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE] = {0};
    read(client_socket, buffer, sizeof(buffer));

    char *username = strtok(buffer, "\n");
    char *password = strtok(NULL, "\n");

    pthread_mutex_lock(&counter_mutex);
    if (is_user_connected(username)) 
    {
        char *message = "Clientul este deja conectat.\n";
        send(client_socket, message, strlen(message), 0);
        pthread_mutex_unlock(&counter_mutex);
        close(client_socket);
        return NULL;
    }

    if (authenticate_user(username, password)) 
    {
        char client_dir[BUFFER_SIZE];
        create_client_directory(client_dir,username);

        // Assign unique client number only after authentication
        int client_number = ++client_counter; 
        printf("Handling client %d %s...\n", client_number, username);
        strcpy(connected_users[connected_count], username);
        client_ids[connected_count] = client_number;
        connected_count++;
        pthread_mutex_unlock(&counter_mutex);

        char *success_message = "Autentificare reusita\n";
        send(client_socket, success_message, strlen(success_message), 0);

        while (1) 
        {
            memset(buffer, 0, BUFFER_SIZE);
            int bytes_read = read(client_socket, buffer, sizeof(buffer));
            if (bytes_read <= 0) 
            {
                break;
            }

            int optiune = atoi(buffer);
            printf("Optiune primita de la client %d: %d\n",client_number, optiune);
            switch (optiune) 
            {
                case 0:
                    disconnect_user(client_number);
                    close(client_socket);
                    return NULL;
                case 1:
                    const char *response = traverse_directory(".", 0);
                    send(client_socket, response, strlen(response), 0);
                    break;
                case 2:
                    const char *response2 = traverse_directory(client_dir, 0);
                    send(client_socket, response2, strlen(response2), 0);
                    break;  
                 case 3:
                    char source_path[BUFFER_SIZE];
                    char destination_path[BUFFER_SIZE];
                    memset(buffer, 0, BUFFER_SIZE);
                    memset(source_path, 0, BUFFER_SIZE);
                    memset(destination_path, 0, BUFFER_SIZE);
                    read(client_socket, buffer, sizeof(buffer));
                    sscanf(buffer, "%s %s", source_path, destination_path);
                    incarcare_fisier(source_path,destination_path,client_socket);
                    break;
                case 4:
                    memset(buffer, 0, BUFFER_SIZE);
                    read(client_socket, buffer, sizeof(buffer));
                    sterge_fisier(buffer, client_socket);
                    break;                  
                case 5:
                    handle_search(client_socket);
                    break;
                case 6:
                    create_snapshot(".", "./output", client_socket, client_number);
                    char snapshot_directory[BUFFER_SIZE];
                    create_snapshots_directory(client_dir, snapshot_directory);
                    create_snapshot_for_client(client_dir, snapshot_directory, client_socket);
                    break;
                default:
                    send(client_socket, "Optiune invalida.\n", 19, 0);
            }
        }
    } 
    else 
    {
        char *fail_message = "Autentificare esuata\n";
        send(client_socket, fail_message, strlen(fail_message), 0);
        pthread_mutex_unlock(&counter_mutex);
    }

    close(client_socket);
    return NULL;
}



int main() 
{
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    pthread_mutex_init(&counter_mutex, NULL); // Initialize mutex

    // Socket setup remains the same
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
    {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) 
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 20) < 0) 
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server running and waiting for connections...\n");

    while (1) 
    {
        int *client_socket = malloc(sizeof(int));
        if ((*client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) 
        {
            perror("Accept failed");
            free(client_socket);
            continue;
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, client_socket) != 0) 
        {
            perror("Failed to create thread");
            free(client_socket);
        }

        pthread_detach(thread_id);
    }

    close(server_fd);
    pthread_mutex_destroy(&counter_mutex); // Destroy mutex
    return 0;
}

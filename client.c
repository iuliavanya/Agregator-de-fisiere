#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 4096


//Functia de vizualizare
void vizualizare(char buffer[], int n, int sock) 
{
    memset(buffer, 0, n);
    read(sock, buffer, n);
    printf("%s", buffer);
}



//Functia de transfer fisiere/directoare
void incarcare_fisier(char buffer[], int sock) 
{
    char source_path[BUFFER_SIZE], destination_path[BUFFER_SIZE];
    printf("Introduceti calea fisierului de incarcat: ");
    scanf("%s", source_path);
    printf("Introduceti calea destinatie unde sa fie incarcat: ");
    scanf("%s", destination_path);

    // Formez cererea pentru incarcare
    snprintf(buffer, 2*BUFFER_SIZE, "%s %s", source_path, destination_path);

    // Trimit calea sursa si destinatie catre server
    send(sock, buffer, strlen(buffer), 0);

    // Primesc raspunsul de la server
    memset(buffer, 0, BUFFER_SIZE);
    read(sock, buffer, BUFFER_SIZE);
    printf("%s", buffer);
}



//Functia de stergere fisiere/directoare
void stergere_fisier(char buffer[], int n, int sock) 
{
    memset(buffer, 0, n);
    read(sock, buffer, n);
    printf("%s", buffer);
}



//Functia de cautare
void search_file(char buffer[], int sock) 
{
    char search_query[BUFFER_SIZE];
    printf("Introduceti numele fisierului de cautat: ");
    scanf("%s", search_query);

    // Trimit cererea de cautare
    char search_request[BUFFER_SIZE + 10];
    snprintf(search_request, sizeof(search_request), "SEARCH:%s", search_query);
    send(sock, search_request, strlen(search_request), 0);

    // Primesc raspunsul de la server
    memset(buffer, 0, BUFFER_SIZE);
    read(sock, buffer, BUFFER_SIZE);
    if (strcmp(buffer, "NOT_FOUND") == 0) 
    {
        printf("Fisierul '%s' nu a fost gasit.\n", search_query);
    } 
    else 
    {
        printf("Fisier gasit: %s\n", buffer);
    }
}



int main() 
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Creare socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        printf("\nSocket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convertire adresa IP
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) 
    {
        printf("\nInvalid address/ Address not supported\n");
        return -1;
    }

    // Conectare la server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    {
        printf("\nConnection Failed\n");
        return -1;
    }

    // Autentificare
    char username[50], password[50];
    printf("Nume utilizator: ");
    scanf("%s", username);
    printf("Parola: ");
    scanf("%s", password);

    // Trimitem username si parola intr-un pachet unic
    char auth_data[BUFFER_SIZE];
    snprintf(auth_data, sizeof(auth_data), "%s\n%s\n", username, password);
    send(sock, auth_data, strlen(auth_data), 0);

    // Primesc raspunsul de la server
    memset(buffer, 0, BUFFER_SIZE);
    read(sock, buffer, BUFFER_SIZE);
    printf("%s", buffer);
    
    if (strncmp("Autentificare reusita", buffer, 21) == 0) 
    {
        while (1) 
        {
            printf("Selectati operatia dorita:\n");
            printf("0. Deconectare\n");
            printf("1. Vizualizare arbore fisiere\n");
            printf("2. Vizualizare arbore fisiere client\n");
            printf("3. Transfer fisiere\n");
            printf("4. Stergere fisiere/directoare\n");
            printf("5. Cautare fisiere\n");
            printf("6. Realizare snapshot\n");
            printf("Introduceti optiunea (0-6): ");

            int optiune;
            scanf("%d", &optiune);

            // Trimit optiunea la server
            char optiune_str[5];
            snprintf(optiune_str, sizeof(optiune_str), "%d", optiune);
            send(sock, optiune_str, strlen(optiune_str), 0);

            // Executam operatia pe baza optiunii
            if (optiune == 0) 
            {
                printf("Deconectare...\n");
                break;
            }
            else if (optiune == 1) 
            {
                vizualizare(buffer, BUFFER_SIZE, sock);
            } 
            else if (optiune == 2) 
            {
                vizualizare(buffer, BUFFER_SIZE, sock);
            } 
            else if (optiune == 3) 
            {
                memset(buffer, 0, BUFFER_SIZE);
                incarcare_fisier(buffer, sock);
            } 
            else if (optiune == 4) 
            {
                printf("Introduceti numele fisierului/directorului de sters: ");
                char nume_fisier[BUFFER_SIZE];
                scanf("%s", nume_fisier);
                send(sock, nume_fisier, strlen(nume_fisier), 0);
                stergere_fisier(buffer, BUFFER_SIZE, sock);
            } 
            else if (optiune == 5) 
            {
                search_file(buffer, sock);
            }
            else if (optiune == 6) 
            {
                printf("Snapshot creat cu succes in directorul de output. \n");
            }
            else 
            {
                printf("Aceasta optiune nu este valida.\n");
            }
        }
    } 

    close(sock);
    return 0;
}

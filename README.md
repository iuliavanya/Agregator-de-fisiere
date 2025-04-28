# Agregator-de-fisiere

# 🌐 Client-Server File Management System

Acest proiect reprezintă o aplicație **client-server** în C pentru gestionarea de fișiere. Utilizatorii se pot autentifica, vizualiza structura directoarelor, transfera fișiere, șterge fișiere/directoare, căuta fișiere și realiza snapshot-uri ale fișierelor existente.

---

## 🛠️ Funcționalități principale

- **Autentificare** utilizator (cu validare dintr-un fișier `users.txt`)
- **Vizualizare** arborele de fișiere general sau doar al clientului
- **Transfer de fișiere** între client și server
- **Ștergere** de fișiere sau directoare recursiv
- **Căutare** fișiere după nume
- **Realizare snapshot** al structurii de fișiere

---

## 📂 Structura fișierelor

- `finalserver.c` — codul sursă pentru **server**
- `finalclient.c` — codul sursă pentru **client**
- `users.txt` — fișierul text care stochează utilizatorii și parolele.

---

## 🧰 Compilare

Compilează serverul și clientul separat folosind `gcc`:

```bash
gcc -o server finalserver.c -lpthread
gcc -o client finalclient.c

# 🚀 Rulare

## 1. Pornirea serverului

```bash
./server
Serverul va asculta pe portul `8080`.

---

## 2. Pornirea clientului

Într-un alt terminal:

```bash
./client

# 👤 Autentificare

La pornire, clientul cere:

- **Nume utilizator**
- **Parolă**

Datele sunt verificate în fișierul `users.txt`.

Dacă autentificarea reușește, utilizatorul poate continua cu operațiunile.

---

# 📋 Operațiuni disponibile

După autentificare, clientul poate alege:

| Cod | Operație                    | Descriere                                          |
|:---:|:-----------------------------|:---------------------------------------------------|
|  0  | Deconectare                  | Închide conexiunea clientului                      |
|  1  | Vizualizare arbore fișiere    | Afișează structura tuturor fișierelor serverului   |
|  2  | Vizualizare arbore client     | Afișează doar fișierele proprii ale clientului     |
|  3  | Transfer fișier               | Transferă un fișier către un director              |
|  4  | Ștergere fișier/director      | Șterge un fișier sau un director recursiv          |
|  5  | Căutare fișier                | Caută un fișier după nume                         |
|  6  | Realizare snapshot            | Creează un snapshot al fișierelor existente       |

---

# 📎 Observații

- Serverul gestionează **clienți multipli** prin **thread-uri**.
- Fiecare client are propriul director (`client_<username>`).
- Snapshot-urile sunt salvate automat atât pe **server**, cât și în directorul **clientului**.
- Protecție **thread-safe** pentru datele globale (de exemplu contorul de clienți) folosind `pthread_mutex_t`.

---

# 🧹 Recomandări suplimentare

- Creează un fișier `users.txt` cu utilizatori și parole, de exemplu:

```plaintext
user1
user2
user3

-Asigură-te că server are permisiuni de scriere în directoarele de lucru.

-În caz de eroare la conexiune, verifică firewall-ul sau porturile deschise.



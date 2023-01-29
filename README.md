# tema3_pso

## Headere

### comune.h

- Define-uri care se folosesc si in server si in client

### logger.h

- Metodele si variabilele folosite pentru logarea operatiilor facute pe server in fisierul log

### linkedlist.h

- Definirea nodului si functii pentru a folosi lista care va tine fisierele si operatia care se executa pe aceasta

## server.c

### Structuri

- WordCount = pentru a numara aparitia unui cuvant in fisier
- Params = pentru a da parametrii functiei pthread_create cand se creaza un client nou (socket fd si index)
- listOfFiles = lista de fisiere cu top 10 cuvinte
- indexOperations = numarul de operatii care se executa intr-un moment
- Node = un nod din lista de fisiere si operatii

### Functii

- Initialize: creare socket, bind cu port si ip
- SendFileList, SendFile, DeleteFile, PutFile, SearchWord, UpdateFile sunt functiile din cerinta
- Execute: Sparge mesajul de la client cu strtok si executa comanda
- HandleClient: Threadul unui client; face recv pe mesajul de la client si da execute
- HandleIncomingConnection: Threadul care asculta daca vin conexiuni
- UpdateList: Threadul care face update la lista de fisiere
- UpdateListRecursive: Functia care populeaza lista de fisiere
- FirstUpdate: Prima populare a listei de fisiere

## client.c

- pe timpul rularii clientului se va scrie lista de comenzi si cum trebuie acestea transmise
- am evitat sa pun spatiu cand transmit comenzile deoarece sparg cu strtok si nu am vrut sa pun ca token spatiul, astfel sa pot face update in fisiere incluzand spatiile 

## Mentiuni

- am separat lista de fisiere prin '\n', prin '\0' nu se mai intelegea bine in client
- fisierul log.txt este cel in care se tin loggurile
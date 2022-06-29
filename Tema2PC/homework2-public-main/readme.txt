//Toma Bogdan-Nicolae
//323CB

/*
    Tema este structurata in 3 fisiere:
-subscriber
-server
-udp

    Subscriberii reprezinta clientii tcp, care deschid un socket pentru a se
conecta cu serverul, caruia ii trimite si id-ul sau, iar in cazul in care 
id-ul este deja conectat serverul inchide conexiunea.

    Clientul poate sa faca urmatoarele:
    -dea subscribe/unsubscribe la anumite topic-uri citite de la stdin si
trimise mai departe la server intr-un buffer
    -sa inchida conexiunea cu serverul cand se citeste "exit" la stdin
    -sa primeasca mesaje de la topic-urile la care este abonat prin server.

    Atunci cand sunt primite date sunt de forma structurii udp_to client care
contine ip-ul si portul clientului udp care trimite mesajul, dar si topic-ul,
type-ul si contentul acestuia. Pentru afisare se analizeaza tipul datelor cu
ajutorul unui switch (0-int,1-short,2-float,3-string). Daca tipul de date este
un numar se face conversia network to host si se afiseaza corespunzator (se 
verifica semnul cand este cazul si alipirea partii intregi la partea zecimala
a numarului pentru float), iar daca tipul de date este string se asigura ca
finalul contentului este null terminator si se afiseaza.

    Serverul: deschide socket-uri pentru conexiunile cu subscriber si cu udp,
si stocheaza toate socket-urile in read_fds, si socket-ul cu valoarea cea mai mare
in fdmax, completeaza ip-ul si portul serverului cu cele date ca parametru la rulare
si realizeaza bind-ul cu tcp si udp. Se stocheaza clientii tcp intr-un array de structura 
tcpClient ce contine socket-ul fiecarui client (-1 daca este deconectat), id-ul, si 
abonarile acestuia.

    Se parcurg toate socket-urile si se verifica tipul acestora:
    -Pentru cand vine o cerere de conexiune tcp pe socket-ul cu listen se obtine
adresa clientului si id-ul acestuia. Apoi, se parcurg clientii tcp pentru a vedea
daca clientul este deja conectat (caz in care se inchide socket-ul) sau daca
clientul a mai fost conectat (pentru a i se salva subscriptiile). Daca nu se
intampla cazurile de mai sus se adauga un nou client si se completeaza campurile
aferente.

    -Pentru cand se citeste de la tastatura, singurul caz fiind atuni cand se
doreste inchiderea serverului, caz in care se opresc toate socket-urile active
si se iese inchide programul.

    -Pentru cand se primesc mesaje de la clientii tcp, care pot fi de 3 tipuri:
0 - se doreste inchiderea conexiunii

subscribe - se parcurg clientii pana se obtine cel de pe socket-ul bun si se 
            verifica prin functia find daca clientul este deja abonat la acel 
            topic, in caz contrar se adauga topicul in vectorul de subscriptii.

unsubscribe - se parcurg clientii ca mai sus si se sterge topicul daca exista
              prin functia erase.

    -Pentru cand se primesc mesaje de la clientii udp, care se parseaza si se
trimit mai departe subscriber-ului cu ip-ul si portul clietntului udp

    Clientii udp: trimit mesaje catre server de 1551 de biti ce contin topic-ul
tipul de date si continutul mesajului.

*/
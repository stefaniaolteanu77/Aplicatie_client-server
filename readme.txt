Olteanu Cristiana-Stefania 322CD

Implementare
    Subscriber
     Creez un socket pentru comunicarea cu serverul si initial trimit pe
    acesta id-ul clientului.Cu multiplexare I/O reusesc sa primesc si 
    de la tastatura comenzile exit, subscribe si unsubscribe, ultimele doua
    le parsez si le pun intr-o structura request. Trimt serverului aceste
    comenzi si afisez mesajele corespunzatoare. De la server primesc mesajul
    ca s-a inchis socketul de comunicare sau ca s-a inchis serverul in 
    sine(disconnect).  

    Server
        Si aici ma folosesc de multimplexare I/O pentru a putea schimba
    intre STDIO, socketul udp, socketul tcp pasiv si socketii tcp activi.

        Pentru a putea lucra eficient cu mesajele care trebuie trimise la
    subscriber si cu subscriberii care se pot conecta si primii mesaje atunci
    dar si deconecta si primii mesajele de la topicurile abonate cu sf setat
    pe 1 dupa ce se reconnecteaza daca isi pastreaza id-ul m-am folosit mult
    de unordered_map pentru a fi mai eficient. 
        Am un unordered_map clients cu cheia id-ul si valoarea file 
    descriptorul unui client pentru a pastra clientii conectati la server.
    Am un unordered_map backlog_messages cu cheia topicul si valorea un vector
    de mesaje trimise pe acel topic pentru a tine evidenta mesajelor ce vor 
    trebui trimise clientiilor deconectati cu sf-ul setat pe 1 ce se reconecteaza
    si trebuie sa isi primeasa mesajele la topic-urile abonate. Am pastrat de 
    asemenea doua unordered_map unul cu cheia topicul si celalalt cu cheia
    id-ul clientului, si valoarea un alt map cu cheia si valoarea un subscriber 
    si sf-ul pentru acel topic respectiv un topic si sf pentru acesta pentru 
    a tine cont de subscriberii online vs cei ce se afla offline.
        Am definit o structura de subscriber care pentru a putea fi folosita
    in unordered_map pe post de cheie a trebuit sa ii implementez modul in care
    se face hash pe elementele de acest tip.
        Flow-ul serverului in momenttul in care se face multiplexare I/O :
          - de la tastatura de poate primi comanda de exit care opreste 
        serverul, inchizand toti socketii deschisi.
          - pe socket-ul de udp se primesc informatiile de la provider
        pe care am ales sa le primesc intr-o structura udp_message
        ca mai apoi sa o convertesc intr-o forma ce trebuie trimisa
        la client punand-o initial intr-o structura tcp_message si
        apoi mesajul in sine  cu sprintf in buffer. Aici pentru clientii ce se
        afla online trimit mesajul iar pentru cei offline si cu sf pe 1 pun
        mesajul in functie de id in  map-ul de backlog_messages, pentru a le
        putea trimite cand se reconecteaza.
        - pe socket-ul pasiv de tcp o sa accept conexiunea cu noii subscriberi
        creand noi socketuri active de tcp. Aici o sa primesc de la subscriber
        id-ul sau si o sa verific daca exista deja cineva conectat cu id-ul 
        respectiv. De asemenea, acesta este momentul in care se vor trimite 
        mesajele din backlog_messages in caz ca acest client a mai fost 
        conectat si a avut vreo subscriptie cu sf setat pe 1. Daca clientul
        se afla in mapul de clienti offline il mut in mapul de clienti online
        la topicurile la care s-a abonat cu sf-urile respective.
        - daca comunicam cu un socket tcp activ. In cazul in care nu mai primim
        nimic de la client inseamna ca acesta s-a deconectat si il voi muta din 
        clientii online in cei offline cu tot ceea ce presupune. Daca in schimb
        am primit informatie de la client aceasta v-a fi o comanda de tip 
        subscribe/unsubscribe si voi aduga/sterge topicul din clientii online




    


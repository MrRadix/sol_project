# sol_project

## Supermarcket
1. Main process
2. Includes client.h, cashier.h, director.h
3. Takes as parameters:
    * K (cashiers number)
    * C (clients number)
    * E (number of clinets entering after C-E end)
    * T (max time for purchases)
    * P (max number of products that a client can purchase)
4. Opens a thread for each
    * Client
    * Cashier
5. Opens a thread for director
6. For each cashier there is a FIFO queue where clients waits for their turn
    * client pushes the read side of a pipe without name on a FIFO queue and waits for his turn doing a read on the pipe
    * cashier owner of that pipe pops a pipe and writes a message 
        * "next" client wake up and start passing products
        * "closing" client wake up and puts himself on another queue
    * While clients are served they put products on another FIFO queue
7. Each cashier at regular intrvalls pushes on a queue information for director

## Client
1. takes as parameters:
    * t time for purchases
    * p number of product to purchase
3. stays in purchasing state for t ms
2. sends to cashier:
    * number of products purchased
    * total time in the supermarket
    * total time in queue
    * number of queue viewed 

## Cashier
1. takes as parameters:
    * id
    * fixed time

## Director
1. Decides if a cash should be closed checking bound values in config.h
2. reads analytics data from cashiers popping from a specific queue 


## Structure:
* supermarket:
    * supermarket.c
* clients:
    * client.c
    * client.h
* cashier:
    * cashier.c
    * cashier.h
* director:
    * director.c
    * director.h
* configuration:
    * config.h
* queue:
    * tsqueue.h
    * tsqueue.c

## Comuntication:
* client -> casher
* client -> director
* casher -> director

## Questions:
1. Il file config.h deve essere incluso oppure deve essere letto come un file di testo normale?
2. Se nel file scrivo i valori di K, C, E, T, P, S come faccio poi a cambiarli passandoli come parametri ad esempio?
3. Se il direttore è un thread come è possibile scrivere il target test2 che dovrebbe avviare il processo direttore e che a sua volta avvia il supermercato?
4. Il controllo della soglia C-E deve essere implementata nel direttore oppure nel supermercato?
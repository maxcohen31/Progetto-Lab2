# Costruzione di Lu e Lv
La costruzione di Lu e Lv tramite alberi binari di ricerca ha rappresentato un bottleneck. Come suggerito nel forum dedicato al progetto 
viene eseguita una visita alternata della MSF. Viene dunque di fatto costruito un unico ABR che rappresenta la componente più piccola. 

# Note sulla cancella_arco
La cancella_arco fa due cose: 
  - Se l'arco non fa parte della MSF lo rimuove dalla tabella hash e dalle liste di adiacenza
  - Se l'arco fa parte della MSF fa una visita alternata e trova la componente più piccola e da essa costruisce l'albero binario di ricerca.
    Per trovare un arco candidato la cancella_arco utilizza la funzione find_min che cerca ricorsivamente un arco che connette le componenti.
        - Se l'arco viene trovato la cancella_arco lo inserisce nella MSF e aggiorna le liste di adiacenza e il costo della MSF.
        - Altrimenti la componente si divide: se il minimo id originale fa parte della componente più piccola, si finisce la visita fatta in precedenza 
          e aggiorniamo la componente più grande con il minimo id trovato. Se l'id fa già parte della componente grande semplicemente si fa l'aggiornamento.


# Sincronizzazione
Per la parte di sincronizzazione il programma adotta un classico schema produttori-consumatori. In particolare qui si ha un solo produttore e un numero di thread consumatori che sarà l'utente a definire dalla linea di comando. Il numero minimo è tre consumatori.
Per proteggere le struture dati e garantire un'effettiva sincronizzazione si utilizzano le seguenti strutture dati:
    - mu-cc_busy: mutex che protegge l'accesso a cc_busy

    - cc_busy: array di booleani. Se cc_busy[i] == true significa che la componente è occupata da un'operazione

    - cv_cc: condition variable utilizzata dai thread che devono attendere affinchè la componente si liberi

    - mut_gHash: array di mutex per proteggere i bucket della hash table

    - update: mutex per proteggere gli aggiornamenti ai vari contatori

# Sequenza operazioni 
- Il thread consumatore preleva un'operazione dal buffer.
- Viene chiamata lock_cc. Il thread acquisisce mu_cc_busy e controlla se una delle componenti è occupata.
- Se una componente è occupata il thread attende su cv_cc. Questa chiamata rilascia mu_cc_busy, permettendo ad altri thread di operare.
- Quando le componenti diventato disponibili i relativi flag di cc_busy vengono messi a true.
- A seconda del campo 'op' si esegue cancella_arco o aggiungi_arco
- Viene chiamata unlock_cc per rilasciare le componenti: acquisisce mu_cc_busy e setta i flag a false e segnala la condition variable con pthread_cond_signal
- Stampa il risultato

### Protezione delle strutture dati
I bucket della hash table sono protetti da mutex associati ad essi. Per un arco si calcola il bucket corrispondente e anche il mutex tramite le operazioni:
    int bucket = hash(u, v, g->hash_table)
    int mu_idx = bucket % g->nmutex
La prenotazione delle componenti è eseguita tramite mu_cc_busy così il controllo della disponibilità e l'assegnazione dei flag può essere eseguito atomicamente.


# Esempio

Supponiamo che il grafo contenga due componenti tra loro indipendenti:  {0, 1, 2} e {5, 6, 7} e che la key degli archi sia differente.

Supponiamo che ci siano due thread: T1 e T2. T1 deve cancellare l'arco (2, 5) mentre T2 deve aggiungere l'arco (5, 6).

- T1 acquisisce mu_cc_busy, controlla che le componenti 0 e 5 siano libere e imposta i flag a true. Poi rilascia mu_cc_busy. T1 ha prenotato le componenti.
- T2 deve prenotare la componente 5; acquisisce mu_cc_busy ma mu_cc_busy[5] = true e quindi si mette in attesa.
- T1 acquisisce mut_gHash[mu_idx], inserisce l'arco nella hash table e rilascia la mutex hash. Aggiorna le liste di adiacenza e le info sulle componenti.
- T1 libera le componenti.
- T2 si sveglia, riacquisisce la mu_cc_busy e ricontrolla la condizione. Se la componente è libera procede.
- T1 e T2 se devono aggiornare le statistiche si alternano sul mutex updates.





/**
 * @file objectstorelib.h
 * @author Raffaele Apetino - Matricola 549220 (r.apetino@studenti.unipi.it)
 * @brief 
 * libreria per client che implementa la REGISTER, STORE, RETRIEVE e DELETE con l'object store
 * @version 0.1 final
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <stdlib.h>

/**
 * @brief 
 * inizia la connessione all'object store, registrando il cliente
 * con il "name" dato
 * 
 * @param name 
 * @return true se connessione avvenuta con successo, false altrimenti
 */
int os_connect(char* name);

/**
 * @brief 
 * viene effettuata la memorizzazione dell'oggetto puntato da block
 * per una lunghezza len, con il nome name
 * 
 * @param name 
 * @param block 
 * @param len 
 * @return true se memorizzazione avvenuta con successo, false altrimenti
 */
int os_store(char* name, void* block, size_t len);

/**
 * @brief 
 * recupera l'oggetto precedentemente memorizzato con nome name
 * 
 * @param name 
 * @return puntatore all'oggetto se ricerca effettuata con successo, NULL altrimenti 
 */
void *os_retrieve(char* name);

/**
 * @brief 
 * cancella l'oggetto di nome name precedentemente memorizzato
 * 
 * @param name 
 * @return true se cancellazione avvenuta con successo, false alrimenti
 */
int os_delete(char* name);

/**
 * @brief 
 * chiude la connessione all'object store
 * 
 * @return true se disconnessione avvenuta con successo, false altrimenti 
 */
int os_disconnect();
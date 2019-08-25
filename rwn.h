/**
 * @file rwn.h
 * @author Raffaele Apetino - Matricola 549220 (r.apetino@studenti.unipi.it)
 * @brief
 * segnatura funzioni readn e writen.
 * ATTENZIONE: da utilizzare solamente quando sono certo della dimensione di lettura e scrittura
 * (ad esempio nel caso in cui mi viene inviata la dimensione)
 * @version 0.5 final
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <stdio.h>

/**
 * @brief 
 * funzione che chiama la sc read assicurandosi di leggere esattamente n 
 */
size_t readn(int fd, void *ptr, size_t n);

/**
 * @brief 
 * funzione che chiama la sc write assicurandosi di scrivere esattamente n
 */
size_t writen(int fd, const void *ptr, size_t n);
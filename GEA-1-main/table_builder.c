#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include "lfsr.h"
#include "table_builder.h"
#include "time.h"

#define NUM_THREADS 12

/**
 * Les vecteurs de base sont représentés entiers de 64 bits
 * 
*/
uint64_t base_T_AC[24] = {
0b1000000000000000000000001001110011110011100110000010011111010101UL,
0b0100000000000000000000001101000001111001110011000001001100001100UL,
0b0010000000000000000000000110100000111100111001100000100110000110UL,
0b0001000000000000000000000011010000011110011100110000010011000011UL,
0b0000100000000000000000001000010000001111001110011000001010000111UL,
0b0000010000000000000000001101110000000111100111001100000110100101UL,
0b0000001000000000000000001111000000000011110011100110000000110100UL,
0b0000000100000000000000000111100000000001111001110011000000011010UL,
0b0000000010000000000000000011110000000000111100111001100000001101UL,
0b0000000001000000000000001000000000000000011110011100110011100000UL,
0b0000000000100000000000000100000000000000001111001110011001110000UL,
0b0000000000010000000000000010000000000000000111100111001100111000UL,
0b0000000000001000000000000001000000000000000011110011100110011100UL,
0b0000000000000100000000000000100000000000000001111001110011001110UL,
0b0000000000000010000000000000010000000000000000111100111001100111UL,
0b0000000000000001000000001001110000000000000000011110011111010101UL,
0b0000000000000000100000001101000000000000000000001111001100001100UL,
0b0000000000000000010000000110100000000000000000000111100110000110UL,
0b0000000000000000001000000011010000000000000000000011110011000011UL,
0b0000000000000000000100001000010000000000000000000001111010000111UL,
0b0000000000000000000010001101110000000000000000000000111110100101UL,
0b0000000000000000000001001111000000000000000000000000011100110100UL,
0b0000000000000000000000100111100000000000000000000000001110011010UL,
0b0000000000000000000000010011110000000000000000000000000111001101UL};

// uint64_t long base_V[8]; inutile, la base de V est 0,...,0 1,0,0,0,0,0,0,0, on itere facilement 0->255
//                                                         0,...,0 0,1,0,0,0,0,0,0,
//                                                         0,...,0 0,0,1,0,0,0,0,0,
//                                                         0,...,0 0,0,0,1,0,0,0,0,
//                                                         0,...,0 0,0,0,0,1,0,0,0,
//                                                         0,...,0 0,0,0,0,0,1,0,0,
//                                                         0,...,0 0,0,0,0,0,0,1,0,
//                                                         0,...,0 0,0,0,0,0,0,0,1,


/**
 * Compare deux elements de type tuple (utilisé pour qsort)
*/
int compare(const void *t1, const void *t2) {
  tuple *tuple1 = (tuple *) t1;
  tuple *tuple2 = (tuple *) t2;
  if (tuple1->out1 == tuple2->out1) {
    return (tuple1->out2 > tuple2->out2) ? 1 : -1;
  } else {
    return (tuple1->out1 > tuple2->out1) ? 1 : -1;
  }
}

/**
 * Construit la table Tab[v], tous les couples t + v
 * Avec t appartient à T_AC
 * v appartient à V fixé
 * 
 * L'espace engendré par T_AC est l'ensemble des vecteurs de la forme :
 * a_0 * v_0 + a_1 * v_1 + a_2 * v_2 + ... + a_23 * v_23
 * avec v_i vecteur de base de T_AC et a_i appartient à {0,1}
 * 
 * On peut donc voir un vecteur de la base T_AC comme un nombre binaire de 24 bits
 * avec le ième bit = 1 si a_i = 1
 * On énumere tous les vecteurs avec un compteur allant de 0 à 2^24 - 1
*/
void buildTableB(uint64_t v) {
  //printf("Construction table %lu\n", v);
  tuple *buffer = malloc(sizeof(tuple) * (1 << 24)); // env 200MB
  
  char *filename = malloc(50 * sizeof(char));
  sprintf(filename, "./tabs/tab_%lu", v); // /!\ le repertoire tabs doit etre créé
  FILE *f = fopen(filename, "wb");
  
  uint64_t curVector;
  Lfsr B;
  init_lfsr(&B, 2);
  uint64_t output1;
  uint8_t output2;
  // On enumere toutes les valeurs possibles de v + t, v fixé et t appartient base_T_AC
  for (unsigned int t = 0; t < 1U << 24; t++) {
    curVector = v; 
    for (int i = 0; i < 24; i++) {
      if ((t >> i) & 1) {
        curVector ^= base_T_AC[i];
      }
    }
    //curVector = v + t = s63 .. s0
    // On initialise le registre B avec v + t
    // On clock l fois, le resultat étant dans output1, output2 (64 + 1 bits)

    lfsr_init(&B, curVector);
    output_65_bits(&B, &output1, &output2);
    buffer[t].out1 = output1;
    buffer[t].out2 = output2;
    buffer[t].t = t;
    if (t == 1) {
      printf("Creation table:\noutput1 : %lu\noutput2 : %d\n", output1, output2);
    }
  }
  qsort(buffer, 1 << 24, sizeof(tuple), compare);
  printf("%s Table %lu\n", (fwrite(buffer, sizeof(tuple) * (1 << 24), 1, f) == 1) ? "Succes" : "Echec", v);
  fclose(f);
  free(buffer);
  free(filename);
}

void *buildTablesRange(void *arguments) {
  param *args = (param *)arguments;
  for (uint64_t v = args->start; v < args->end; v++) {
    buildTableB(v);
  }
  pthread_exit(NULL);
}


/**
 * Distribue le calcul de création de tables parmi les threads (définis par NUM_THREADS) 
*/

void buildTables() {
  pthread_t threads[NUM_THREADS];
  param *p[NUM_THREADS]; // liste des parametres pour les threads
  int ret;
  uint64_t curStart = 0;
  uint64_t curEnd = 0;
  for (int t = 0; t < NUM_THREADS; t++) {
    curStart = curEnd;
    curEnd += (256 / NUM_THREADS);
    curEnd += (t < (256 % NUM_THREADS)) ? 1 : 0; // les premiers threads s'occupent du reste de la division
    p[t] = malloc(sizeof(param));
    p[t]->start = curStart;
    p[t]->end = curEnd;
    printf("Appel thread #%d\nstart : %lu\nend : %lu\n", t, curStart, curEnd);
    ret = pthread_create(&threads[t], NULL, buildTablesRange, (void *) p[t]);
    if (ret) {
      printf("ERREUR CREATION THREAD ERR#%d\n", ret);
      exit(-1);
    }
  }
  for (int t = 0; t < NUM_THREADS; t++) {
    pthread_join(threads[t], NULL);
    free(p[t]);
  }

}

/**
 * Au lieu d'utiliser un compteur, 0 -> 2^24 -1
 * On itere sur les chaines binaires via un grayCode
 * (maniere d'enumerer les chaines binaires tel que deux chaines binaires consecutives
 *  diffèrent d'un seul bit [distance de hamming = 1])
 * Ainsi à partir d'un vecteur t + v, on calcule le vecteur suivant t' + v en xorant le seul vecteur de base
 * qui differe.
 * Ex :
 * GrayCode ...001 -> 011 -> 010 ...
 * On obtient le 2e vecteur en xorant le 1er vecteur et le 2eme vecteur de base (indice ou les bits diffèrent entre la 1ere et 2e chaine)
 * On obtient le 3e vecteur en xorant le 2eme vecteur et le 3eme vecteur de base (indice ou les bits diffèrent entre la 2e et 3e chaine)
 * 
 * Ainsi a chaque iteration on ne xor qu'un seul vecteur de base.
 * 
 * BinaryToGray(num) = num ^ (num >> 1)
 * 
 * Temps d'execution environ 8% plus court
*/

// void buildTableBGrayCode(uint64_t v) {
//   tuple *buffer = malloc(sizeof(tuple) * (1 << 24)); // env 200MB
  
//   char *filename = malloc(50 * sizeof(char));
//   sprintf(filename, "./tabs/tab_%lu", v); // /!\ le repertoire tabs doit etre créé
//   FILE *f = fopen(filename, "wb");
  
//   Lfsr B;
//   init_lfsr(&B, 2);
//   uint64_t output1;
//   uint8_t output2;

//   long curVector = v;
//   unsigned int grayT = 0;
//   unsigned int newGrayT;
//   buffer[0].out1 = 0UL;
//   buffer[0].out2 = 0;
//   buffer[0].t = 0U;
//   // On enumere toutes les valeurs possibles de v + t, v fixé et t appartient base_T_AC
//   for (unsigned int t = 1; t < 1U << 24; t++) {
//     newGrayT = t ^ (t >> 1);
//     grayT ^= newGrayT; // grayT contient la difference entre les deux iterations
//     for (int i = 0; i < 24; i++) {
//       if (grayT & 1){
//         curVector ^= base_T_AC[i];
//         break;
//       }
//       grayT >>= 1;
//       if (i == 23) {printf("erreur grayCode 2\n%d", t); exit(1);}
//     }
//     grayT = newGrayT;
//     lfsr_init(&B, curVector);
//     output_65_bits(&B, &output1, &output2);
//     buffer[t].out1 = output1;
//     buffer[t].out2 = output2;
//     buffer[t].t = t;
//   }
//   qsort(buffer, 1 << 24, sizeof(tuple), compare);
//   printf("%s Table %lu\n", (fwrite(buffer, sizeof(tuple) * (1 << 24), 1, f) == 1) ? "Succes" : "Echec", v);
//   fclose(f);
//   free(buffer);
//   free(filename);
// }


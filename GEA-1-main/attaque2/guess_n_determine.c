#include <stdio.h>
#include <stdlib.h>
#include "guess_n_determine.h"

#define NB_MONOMES_F 33
const uint64_t fonction_f[NB_MONOMES_F] = {
    0b01100101, 0b01101001, 0b01100011, 0b01100110, 0b01001101, 0b01011010, 0b01101010, 0b00010101,
    0b00001101, 0b00001011, 0b01000101, 0b00010011, 0b01000011, 0b01000110, 0b01100100, 0b00101001,
    0b01010010, 0b00100110, 0b00001001, 0b00100001, 0b00001010, 0b00100010, 0b01000010, 0b00000101,
    0b00000010, 0b00001100, 0b00100100, 0b01000100, 0b00110000, 0b01100000, 0b00000100, 0b00001000,
    0b00100000
};

void printBin(uint64_t n) {
  for (int i = 0; i < 64; i++) {
      printf("%c", ((n >> (63 - i)) & 1) ? '1' : '0');
  }
  printf("\n");
}

void printMonomes(monomes *listeMonome) {
  while (listeMonome != NULL) {
    printBin(listeMonome->mono);
    listeMonome = listeMonome->suivant;
  }
}

void freeMonomes(monomes *listeMonome) {
  monomes *tmp;
  while (listeMonome != NULL) {
    tmp = listeMonome;
    listeMonome = listeMonome->suivant;
    free(tmp);
  }
}

monomes* copyMonomes(monomes *src) {
  monomes *headDst = malloc(sizeof(monomes));
  monomes *tmp = headDst;
  while (src != NULL) {
    tmp->mono = src->mono;
    tmp->suivant = src->suivant;
    src = src->suivant;
    tmp = src;
  }
  return headDst;
}

/**
 * Renvoie le produit des monomes
 * Liste1 va être free après l'appel de cette fonction, liste2 va être réutilisée
 * 
*/
monomes* produitMonomes(monomes *liste1, monomes *liste2) {
  if (liste1 == NULL) {
    // liste2 est listeMonome dans la fonction substitue, on ne veut pas la modifier.
    return copyMonomes(liste2);
  }
  if (liste2 == NULL) {
    return liste1;
  }
  monomes *resultat = malloc(sizeof(monomes));
  monomes *teteResultat = resultat;
  monomes *curListe2 = liste2;
  while (liste1 != NULL) {
    while (curListe2 != NULL) {
      resultat->mono = (uint64_t) (liste1->mono | curListe2->mono);
      if (curListe2->suivant != NULL) {
        resultat->suivant = malloc(sizeof(monomes));
        resultat = resultat->suivant;
      }
      curListe2 = curListe2->suivant;
    }
    curListe2 = liste2;
    if (liste1->suivant != NULL) {
        resultat->suivant = malloc(sizeof(monomes));
        resultat = resultat->suivant;
      }
    liste1 = liste1->suivant;
  }
  return teteResultat;
}

void addMonome(monomes **listeMonome, uint64_t val) {
  monomes *newMonome = malloc(sizeof(monomes));
  newMonome->mono = val;
  newMonome->suivant = *listeMonome;
  *listeMonome = newMonome;
}

monomes* appendMonome(monomes *listeMonome1, monomes *listeMonome2) {
  if (listeMonome1 == NULL) {
    return listeMonome2;
  }
  if (listeMonome2 == NULL) {
    return listeMonome1;
  }
  monomes *headListe1 = listeMonome1;
  while (listeMonome1->suivant != NULL) {
    listeMonome1 = listeMonome1->suivant;
  }
  listeMonome1->suivant = listeMonome2;
  return headListe1;
}

// Prend en entrée pour chacune des entrées de la fonction f
// L'entier de 64 bits représentant sa combinaison linéaire en fonction des bits de l'état initial.
// Fais la substitution dans la fonction f et renvoie un tableau le tableau des monomes
// Ex : tour-1 : {x0 : 0110, x1 : 1010, .., x6 : 0001}
// x0 = r1 + r2, x1 = r0 + r2, .., x6 = r3
monomes* substitue(uint64_t *tableau) {
  monomes *listeMonomes[7] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
  for (int k = 0; k < 7; k++) {
    // passer de la representation combinaison linéaire à tableau de monome
    uint64_t combinaison = tableau[k];
    for (int shift = 0; shift < 64; shift++) {
      if ((combinaison >> shift) & 1UL) {
        addMonome(&listeMonomes[k], 1UL << shift);
      }
    }
  }
  monomes *resultat = NULL;
  monomes *oldResultat = NULL;
  monomes *resultatFinal = NULL;
  // listeMonomes contient la liste des monomes pour chaque entrée de la fonction f
  // faire les produits de la fonction f
  for (int k = 0; k < NB_MONOMES_F; k++) {
    // Pour chaque monome de la fonction f
    for (int shift = 0; shift < 7; shift++) {
      if ((fonction_f[k] >> shift) & 1UL) {
        oldResultat = resultat;
        resultat = produitMonomes(resultat, listeMonomes[shift]);
        free(oldResultat);
      }
    }
    resultatFinal = appendMonome(resultatFinal, resultat);
    resultat = NULL; 
  }
  for (int k = 0; k < 7; k++) {
    freeMonomes(listeMonomes[k]);
    listeMonomes[k] = NULL;
  }
  return resultatFinal;
}


int main(int argc, char const *argv[])
{
  // RAPPEL : INITIALISER LES POINTEURS A NULL !!!!!!!!!!!!!!!!!!!!!!!!
  uint64_t *tableau = malloc(sizeof(uint64_t) * 7 * 1);

  // x0 = r0 + r1
  tableau[0] = 0b100000000UL;
  // x1 = r0 + r2
  tableau[1] = 0b10000000UL;
  // x2 = r2 + r3 + r4 
  tableau[2] = 0b1000000UL;
  tableau[3] = 0b100000UL;
  tableau[4] = 0b10000UL;
  tableau[5] = 0b1000UL;
  tableau[6] = 0b100UL;

  monomes *result = substitue(tableau);

  printMonomes(result);

  free(tableau);
  freeMonomes(result);

  return 0;
}

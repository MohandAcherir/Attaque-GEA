#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// les bits pour f dans S
const int select_S[7] = {3, 12, 22, 38, 42, 55, 63};

// La fonction f(x0, x1, x2, x3, x4, x5, x6) = tab[Somme x(i)* (2^i)]
const unsigned long long int tab_f[128] = {0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1};



unsigned long long int get(unsigned long long int n) {
	unsigned long long int r = 0;
	unsigned long long int x = 1;
	
	for(int i = 0 ; i < 7 ; i++) {
		r += ((n >> (63-select_S[i])) & 1)*x;
		x *= 2;
	}
	return r;
}

// Fonction de recup de cle
unsigned long long int key(unsigned long long int S, unsigned long long int IV, unsigned long long int dir) {
	unsigned long long int S97 = S; //apres l'initialisation


	// Calculer de-clocker pour les 128 : '0'
	for(int i = 0 ; i < 128 ; i++) {
		unsigned long long int b0 = S97 & 1;
		
		S97 = S97 >> 1;
		S97 = S97 & 9223372036854775807ULL; // 2**63 - 1

		unsigned long long int a0 = tab_f[get(S97)];
		S97 += (a0 ^ b0) * 9223372036854775808ULL; // 2**63
	}

// faire les 33 premiers clockings IV, dir

	unsigned long long int 	S33 = 0;

	for(int i = 0 ; i < 33 ; i++) {
		unsigned long long int a = 0;
		unsigned long long int f = tab_f[get(S33)];
	
		if(i == 32) {
			a = dir ^ f ^ ((S33 & (1ULL<<63))>>63);
		}
		else if(i < 32){
			a = ((IV>>i)&1) ^ f ^ ((S33 & (1ULL<<63))>>63);
		}
		S33 = (S33 << 1) | a;
	}


	unsigned long long int K = 0;
	unsigned long long int r = 1;

	for(unsigned long long int i = 0 ; i < 64 ; i++) {
		unsigned long long int f = tab_f[get(S33)];
		unsigned long long int a = f ^ ((S33 & (1ULL<<63))>>63);
	
		// Clocker 1 seule fois
		S33 = S33 << 1;
		S33 = S33 | ((S97>>(63-i)) & 1);
	
		K += ((S33 & 1) ^ a) * r; // k(i) * 2**i
		r *= 2;
	}
	printf("Key: %llu\n", K);
		
	return K;
}


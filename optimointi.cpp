//g++ optimointi.cpp -pthread -o optimointi && ./optimointi

/*Tekijä: Benjamin Säämäki
Toiminta: Ohjelma käynnistää pää-säikeen lisäksi 4 suoritus-säijettä jotka rekursiivisen function avulla
käyvät lävitse ratkaisuja löytääkseen optimaalisemman ratkaisun.
Pääsäije jää odottamaan käyttäjän syötettä käynnistettyään muut säikeet, kun se saa syötteen se
ilmoittaa muille säikeille että nyt on lopetettava, ja odottaa että muut säikeet pysähtyvät,
sitten se katsoo mikä muista säikeistä on löytänyt optimaalisimman ratkaisun ja tulostaa sen.
 */
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<cstring>
#include<cstdio>
#include<vector>
#include<thread>

char cells_string[10000] = 
	"0	84	86	86	91	87	89	98	88	87	91	82	100	96	86\n"
	"96	0	85	76	78	80	77	86	75	86	79	75	100	86	79\n"
	"72	68	0	68	73	65	60	65	78	73	65	64	100	72	71\n"
	"100	84	94	0	95	97	87	95	83	93	94	89	95	83	87\n"
	"100	84	91	87	0	92	87	89	79	81	80	80	96	90	87\n"
	"99	86	84	91	93	0	78	96	84	88	85	83	100	91	79\n"
	"95	68	64	75	78	87	0	77	77	78	100	64	84	73	66\n"
	"100	80	79	75	89	86	75	0	87	84	86	82	98	100	86\n"
	"86	83	91	80	76	80	85	83	0	84	83	87	87	95	100\n"
	"100	82	81	76	80	77	79	85	79	0	81	75	91	78	76\n"
	"97	81	87	93	93	82	100	95	84	84	0	76	83	85	77\n"
	"100	89	91	82	87	88	87	98	87	90	85	0	99	99	93\n"
	"90	73	100	75	72	71	61	82	63	67	73	69	0	85	81\n"
	"94	64	59	55	65	59	62	75	81	58	63	56	100	0	66\n"
	"87	74	79	67	75	70	72	72	100	64	71	79	86	77	0";

int cell_count = 0;
int row_lenght = 0;
int cell_buffer[10000];

//Tämä functio parsii lähtötaulukon cell_bufferiin, joka annetaan parametrina functiolle
int read_values_from_string(int* cell_buffer){
	int tmp_i = 0;
	char* buff = cells_string;
	while(buff[tmp_i++] != 0x00);

	int* p_tmp_buff = cell_buffer;
	char tmp_string[10];
	int string_index = 0;

	for(int i = 0;i < tmp_i;i++){
		if(buff[i] == ' '){
			cell_count++;
			tmp_string[string_index] = 0x00;
			*p_tmp_buff = atoi(tmp_string);
			p_tmp_buff++;
			string_index = 0;
		}
		
		else if(buff[i] >= '0' && buff[i] <= '9'){
			tmp_string[string_index] = buff[i];
			string_index++;
		}
		else if(i == tmp_i - 1){
			cell_count++;
			if(row_lenght == 0)
				row_lenght = cell_count;

			tmp_string[string_index] = 0x00;
			*p_tmp_buff = atoi(tmp_string);
			p_tmp_buff++;
			string_index = 0;
		}

		else if(string_index > 0){
			cell_count++;
			if(row_lenght == 0 && buff[i] == '\n')
				row_lenght = cell_count;

			tmp_string[string_index] = 0x00;
			*p_tmp_buff = atoi(tmp_string);
			p_tmp_buff++;
			string_index = 0;
		}
	}
	printf("cell count = %d\n",cell_count);
	printf("row lenght = %d\n",row_lenght);
	return cell_count;
}

//Tämän muuttujan avulla main thread informoi säikeitä kun on aika lopettaa rekursio
bool kill_thread = false;	

//Thread_object sisältää tarvittavat muuttujat ja toiminnallisuuden että kukin
//thread voi itsenäisesti käydä lävitse ratkaisuja
class Thread_object{
public:	
	int best_sum = 0;	//Paras löydetty tulos
	int sum = 0;		//Tämän hetkinen tulos
	int reserved_cells[15][2];	//Tämän hetkinen tutkittava ratkaisu
	int best_reserved_cells[15][2];	//Paras löydetty ratkaisu

	//ladder muuttuja viittaa taulukon rivinumeroon ja functio swappaa kyseisen rivin valittuja soluja
	//kaikkien alempien rivien valittujen solujenkanssa
	void recursion(int ladder){
		if(kill_thread)	//jos main thread asettaa muuttujan kill_thread todeksi lopetetaan rekursio
			return;
		if(ladder < row_lenght-1){ //Tullaan viimeiselle riville taulukossa ei ruveta swäppäämään soluja
			//Usein käytettävien arvojen tallentaminen muuttujiin algoritmin nopeuttamiseksi
			int old_sum = sum;
			int tmp_cell = reserved_cells[ladder][0];
			int tmp_row_index = ladder*row_lenght;
			int tmp_sum = sum - cell_buffer[tmp_row_index + tmp_cell];
			//i-muuttujaa käytetään toisen swapattavista soluista rivin määrittämiseen
			//toisen swapattavan solun rivinumero määritellään ladder-muuttujalla
			for(int i = ladder+1; i < row_lenght;i++){
				sum = tmp_sum;
				//tässä estetään ettei samalle riville tule valituksi sama solu kahteen kertaan
				if( tmp_cell == reserved_cells[i][1]){
					continue;
				}
				//summan laskenta + swappaukset
				sum -= cell_buffer[i*row_lenght + reserved_cells[i][0]];
				sum += cell_buffer[i*row_lenght + tmp_cell];
				sum += cell_buffer[tmp_row_index + reserved_cells[i][0]];
				reserved_cells[ladder][0] = reserved_cells[i][0];
				reserved_cells[i][0] = tmp_cell;
				//nyt kun swappaus on tehty lähdetään hierarkiassa alaspäin kokeilemaan uusia ratkaisuja
				//kutsutaan rekursiota seuraavalla rivinumerolla
				recursion(ladder + 1);

				//katsotaan onko tulos parantunut ja tallennetaan uusi tulos jos on
				if(sum > best_sum){
					best_sum = sum;
					for(int i3 = 0; i3 < row_lenght;i3++){
						best_reserved_cells[i3][0] = reserved_cells[i3][0];
						best_reserved_cells[i3][1] = reserved_cells[i3][1];
					}
				}
				//Alkuperäisen ratkaisun palautus
				reserved_cells[i][0] = reserved_cells[ladder][0];
				reserved_cells[ladder][0] = tmp_cell;
			}
			sum = old_sum;
		}
	}
};

//Funktio jonka jokainen threadi suorittaa.
//Jokainen thread saa oman Thread_objectin hallintaansa
void thread_function(Thread_object* to){
	to->recursion(0);
}

int THREAD_COUNT = 4;
int main(int argc, char *argv[]){
	cell_count =  read_values_from_string(cell_buffer);

	Thread_object threads[4];
	
	//Jokaiselle Thead_objectelle alustetaan eri alku ratkaisu
	for(int ti = 0; ti < THREAD_COUNT;ti++){
		int start_index = 4 + ti * 2;
		for(int i = 0; i < row_lenght - row_lenght % 2;i += 2){
			threads[ti].reserved_cells[i][0] = (i + start_index )% row_lenght;
			threads[ti].reserved_cells[i][1] = (i + start_index +1)% row_lenght;
			threads[ti].reserved_cells[i+1][0] = (i + start_index )% row_lenght;
			threads[ti].reserved_cells[i+1][1] = (i + start_index +1)% row_lenght;
		}
		if(row_lenght % 2 > 0){
			threads[ti].reserved_cells[row_lenght-1][1] =
				((start_index + row_lenght)-2) % row_lenght;
			threads[ti].reserved_cells[row_lenght-1][0] =
				((start_index + row_lenght)-1)  % row_lenght;
			threads[ti].reserved_cells[row_lenght-2][1] =
				((start_index + row_lenght)-1) % row_lenght;;
		}
		//Alkuratkaisujen tulostus
		printf("Thead %d start values: ,", ti);
		for(int i = 0; i < row_lenght;i++){
			printf("%d,", threads[ti].reserved_cells[i][0]);
			printf("%d,", threads[ti].reserved_cells[i][1]);
		}
		printf("\n");
		//Alkuratkaisun summan tallennus
		threads[ti].sum = 0;
		for(int i = 0; i < row_lenght;i++){
			threads[ti].sum += cell_buffer[i*row_lenght +
										   threads[ti].reserved_cells[i][0]];
			threads[ti].sum += cell_buffer[i*row_lenght +
										   threads[ti].reserved_cells[i][1]];
		}
	}

	std::thread thread_array[4];	//Threadit

	//Threadien käynnistys
	for(int i = 0; i < THREAD_COUNT;i++){
		thread_array[i] = std::thread(thread_function,&threads[i]);
	}
    char p_tmp_char[20];
    printf("Press enter to stop recursion.\n");
	//Odotetaan käyttäjän syötettä, jolloin tiedetään lopettaa threadien suoritus
    fgets(p_tmp_char,20,stdin);
	kill_thread = true;

	//Odotetaan että threadit pysähtyvät hallitusti
	for(int i = 0; i < THREAD_COUNT;i++){
		thread_array[i].join();
	}
	Thread_object* top = &threads[0];
	//Katsotaan mikä threadeista lyösi parhaimman ratkaisun
	for(int i = 1; i < THREAD_COUNT;i++){
		if(threads[i].best_sum > top->best_sum){
			top = &threads[i];
		}
	}
	//Debug-arrayn alustus
	int debug_array[row_lenght];
	for(int i = 0; i < row_lenght;i++){
		debug_array[i] = 0;
	}
	//Legacy syistä paras ratkaisu kopioidaan toiseen arrayhin
	for(int i = 0; i < row_lenght;i++){
		top->reserved_cells[i][0] = top->best_reserved_cells[i][0];
		top->reserved_cells[i][1] = top->best_reserved_cells[i][1];
		debug_array[top->reserved_cells[i][0]]++;
		debug_array[top->reserved_cells[i][1]]++;
	}
	
	//-------------Tulostukset-----------------------------------------------
	//debug arraystä näkee joka sarakkeen valittujen solujen määrän siltä varalta että jotain on menny pieleen
	printf("debug_array: ");
	for(int i = 0; i < row_lenght;i++){
		printf("%d,",debug_array[i]);
	}
	printf("\n\n");
	
	int irow = 0;
	for(int i = 0; i < cell_count;i++){

		int magnitude = 0;
		int tmp_value = cell_buffer[i];
		while(tmp_value > 9){magnitude++;tmp_value /= 10;}

		if(top->reserved_cells[irow][0] == i - irow* row_lenght ||
		   top->reserved_cells[irow][1] == i - irow* row_lenght){
			printf("[");
		}
		else{
			printf(" ");
		}
		tmp_value = 2 - magnitude;
		while(tmp_value-- > 0){
			printf(" ");
		}
		printf("%d",cell_buffer[i]);

		if(top->reserved_cells[irow][0] == i - irow* row_lenght ||
		   top->reserved_cells[irow][1] == i - irow* row_lenght){
			printf("]");
		}
		else{
			printf(" ");
		}
		if(i%15 == 14){
			printf("\n");irow++;
		}
			
	}
	printf("\nresult: %d\n",top->best_sum);
}


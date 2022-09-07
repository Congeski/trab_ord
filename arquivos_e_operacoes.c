#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>  // o que seria isso?

#define COMP_REG 64
#define COMP_CAMPO 60
#define DELIM_STR '|'

int proximoRegistro(char registro[COMP_REG], FILE *file); //percorre os registros??

struct {
  int topo_ped; // o nosso trabalho utiliza LED
} cabecalho;


int byteoffset (int RRN){
    return RRN * COMP_REG + sizeof(cabecalho);
}


int ler_campos(char campo[], int tamanho, FILE *file){
    int i = 0;
    char k = fgetc(file);

    while ( k != DELIM_STR){
        if(feof(file)){
            return -1;
        }

        if (i < tamanho -1){
            campo[i++] = k;
        }
      k = fgetc(file);
    }

    campo[i] = 0;
    return i;
}


int ler_linha(char campo[], int tamanho, FILE *file){
    int i = 0;
    char k = fgetc(file);

    while ( k != '\n'){
        if (feof(file)){  //FUNCAO IGUAL A DE CIMA, NÃO PODERIA USAR UM AND??
            return -1;
        }
        if ( i < tamanho - 1){
            campo[i++] = k;
        }

        k = fgetc(file);
    }

    campo[i] = 0;
    return i;
}


void importacao(char nomeArqImport[]){
    FILE *arquivoOriginal;
    FILE *arquivoDados;
    int cabecalho = -1;

    arquivoOriginal = fopen(nomeArqImport, "r");
    arquivoDados = fopen("dados.dat", "w"); //mudar o nome do arquivo

    if ( arquivoOriginal == NULL || arquivoDados == NULL){
        fprintf(stderr, "ERRO NA ABERTURA DOS ARQUIVOS\n ENCERRANDO PROGRAMA\n"); // printf diferente, pq??
        exit(EXIT_FAILURE);
    }

    fwrite(&cabecalho, sizeof(int), 1, arquivoDados);
    char buffer[COMP_REG];

    while (!feof(arquivoOriginal)){
        int sucesso = proximoRegistro(buffer, arquivoOriginal);

        if (sucesso == 1){
            fwrite(buffer, sizeof(char), COMP_REG, arquivoDados);
        }
    }

    fclose(arquivoOriginal);
    fclose(arquivoDados);
}


int proxReg(char registro[COMP_REG], FILE *file){
    int contPipe = 0, tam = 0;  //contPipe???
    char c;

    memset(registro, '\0', COMP_REG); //????

    while (contPipe < 4 && tam < COMP_REG){
        c = fgetc(file);

        if ( c == EOF){
            return -1;
        }

        registro [tam] = c;

        if ( c == DELIM_STR){
            contPipe += 1;
        }

        tam += 1;
    }

    return 1;
}


int busca( char *chave, FILE *file){
    int RRN = 0;
    char campo[COMP_CAMPO];

    fseek(file, sizeof(cabecalho), SEEK_SET);

    while (ler_campos(campo, 7, file) > -1){ //da onde vem esse 7?
        if( strcmp(campo, chave) == 0){
            fseek(file, -7, SEEK_CUR);
            return RRN;
        }

        fseek(file, COMP_REG -7, SEEK_CUR);
        RRN ++;
    }
    return -1;
}



int PED (file, int RRN){
    fseek(file, byteoffset(RRN), SEEK_SET);

    if (fgetc(file) != '*'){
        printf("\nERRO NA PED");
        //exit(1);  ja estava como comentario
    }

    fread(&RRN, sizeof (int), 1, file);
    fseek(file, -1 *(long)(sizeof(char) + sizeof(int)), SEEK_CUR);
    return RRN;
}


void operacoes(char *argv){
    FILE *dados;
    FILE *operacoesFile;

    char campo[COMP_CAMPO];
    char buffer[COMP_REG];
    char def_op;
    char chave[7];
    int RRN;

    operacoesFile = fopen(argv, "rb");
    dados = fopen("dados.dat", "r+b");

    while((def_op = fgetc(operacoesFile)) != EOF){
        buffer [0] = 0;
        fgetc(operacoesFile);

        switch(def_op){
            case 'b':{
                ler_linha(chave, 7, operacoesFile);
                RRN = busca(chave, dados);
                printf("\nREGISTRO DE CHAVE %s\n", chave);
                if (RRN == -1){
                    printf("REGISTRO NÃO ENCONTRADO");
                    break;
                }

                for (int i = 0, i < 4, i++){
                    ler_campos(campo, COMP_CAMPO, dados);
                    strcat(buffer, campo); // strcat função
                    strcat(buffer, DELIM_STR);
                }
             printf("%s; RRN = %d, byte-offset %d\n",buffer, RRN, byteoffset(RRN));
             break;
            }

            case 'i':
                ler_linha(buffer, COMP_REG, operacoesFile);
                strncpy(chave, buffer, 6); //função que não conheco/lembro
                printf("\nINSERÇÃO DO REGISTRO DE CHAVE %s", chave);
                if (busca(chave, dados) != -1){
                    printf("ERRO: CHAVE %s JA EXISTE NO REGISTRO", chave);
                    break;
                }

                rewind (dados);
                fread(&cabecalho, sizeof(cabecalho), 1, dados);
                RRN = cabecalho.topo_ped;
                if(RRN == -1){
                    fseek(dados, 0, SEEK_END);
                    fwrite(buffer, sizeof(char), COMP_REG, dados);
                    printf("\nLOCAL DE INSERÇÃO - FIM DO ARQUIVO\n");
                }
               else{
                cabecalho.topo_ped = PED(dados, RRN);
                fwrite(buffer, sizeof(char), COMP_REG, dados);
                rewind(dados);
                fwrite(&cabecalho, sizeof(cabecalho), 1, dados);
                printf("\nLOCAL : RRN = %d(byte-offset %d)[reutilizado]\n", RRN, byteoffset(RRN));
               }
               break;

            case 'r':
                ler_linha(chave, 7, operacoesFile);
                printf("\nREMOÇÃO DO REGISTRO %s\n", chave);
                rewind (dados);

                fread(&cabecalho, sizeof(cabecalho), 1, dados);

                RRN = busca(chave, dados);
                if(RRN == -1){
                    puts("ERRO : registro nao encontrado!");
                    break;
                }
                fputc('*', dados);
                fwrite(%cabecalho.topo_ped, sizeof(int), 1, dados);
                cabecalho.topo_ped = RRN;

                rewind(dados);
                fwrite(&cabecalho, sizeof(cabecalho), 1, dados);

                printf("REMOVIDO\n\n");
                printf("Posicao : RRN = %d (byte-offset %d)\n", RRN, byteoffset(RRN));
                break;

            default:
                printf("\nNAO FOI POSSIVEL REMOVER");

                fseek(dados, 0, SEEK_END);
                //linha 280
        }
    }
}
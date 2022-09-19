#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#define COMP_REG 64 //tamanho do registro
#define COMP_CAMPO 60 //tamanho do campo (regsitro menos o cabeçalho)

int proxReg(char registro[COMP_REG], FILE *file); 

struct {
  int topo_ped; // o nosso trabalho utiliza LED
} cabecalho;


int byteoffset (int RRN){
    return RRN * COMP_REG + sizeof(cabecalho);
}


int ler_campos(char campo[], int tamanho, FILE *file){
    int i = 0;
    char c = fgetc(file);

    //Leitura dos caracteres ate atingir o delimitador('|')
    while ( c != '|'){

        if(feof(file)){ // Caso o fim do arquivo for alcancado antes do fim do campo
            return -1;
        }

        if (i < tamanho -1){
            campo[i++] = c;
        }

      c = fgetc(file);
    }

    campo[i] = 0;
    return i;
}


int ler_linha(char campo[], int tamanho, FILE *file){ 
    int i = 0;
    char c = fgetc(file);

    //Leitura dos caracteres ate atingir o delimitador('\n')
    while ( c != '\n'){

        if (feof(file)){  
            return -1;
        }
        if ( i < tamanho - 1){
            campo[i++] = c;
        }

        c = fgetc(file);
    }

    campo[i] = 0;
    return i;
}


void importacao(char nomeArqImport[]){ //  ./programa -i nome_arquivo_importacao.txt       
    FILE *arquivoOriginal;
    FILE *arquivoDados;
    int cabecalho = -1;

    arquivoOriginal = fopen(nomeArqImport, "r");
    arquivoDados = fopen("games.txt", "w"); 

    if ( arquivoOriginal == NULL || arquivoDados == NULL){
        fprintf(stderr, "ERRO NA ABERTURA DOS ARQUIVOS\n ENCERRANDO PROGRAMA\n"); 
        exit(EXIT_FAILURE);
    }

    fwrite(&cabecalho, sizeof(int), 1, arquivoDados);
    char buffer[COMP_REG];

    while (!feof(arquivoOriginal)){
        int sucesso = proxReg(buffer, arquivoOriginal);

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

    memset(registro, '\0', COMP_REG); //colocar o \0 até dar o tamanho do registro

    while (contPipe < 4 && tam < COMP_REG){
        c = fgetc(file);

        if ( c == EOF){
            return -1;
        }

        registro [tam] = c;

        if ( c == '|'){
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

    while (ler_campos(campo, 7, file) > -1){ 
        if( strcmp(campo, chave) == 0){
            fseek(file, -7, SEEK_CUR);
            return RRN;
        }

        fseek(file, COMP_REG -7, SEEK_CUR);
        RRN ++;
    }
    return -1;
}



int PED (FILE *file, int RRN){
    fseek(file, byteoffset(RRN), SEEK_SET);

    if (fgetc(file) != '*'){
        printf("\nERRO NA PED");
        //exit(1);  ja estava como comentario
    }

    fread(&RRN, sizeof (int), 1, file);
    fseek(file, -1 *(long)(sizeof(char) + sizeof(int)), SEEK_CUR);
    return RRN;
}


void operacoes(char *argv){ // ./programa -e nome_do_arquivo.txt     para realizar as operações abaixo
    FILE *dados;
    FILE *operacoesFile;

    char campo[COMP_CAMPO];
    char buffer[COMP_REG];
    char def_op;
    char chave[7];
    int RRN;

    operacoesFile = fopen(argv, "rb");
    dados = fopen("games.txt", "r+b");

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

                for (int i = 0; i < 4; i++){
                    ler_campos(campo, COMP_CAMPO, dados);
                    strcat(buffer, campo); // strcat função
                    strcat(buffer, "|");
                }
             printf("%s; RRN = %d, byte-offset %d\n", buffer, RRN, byteoffset(RRN));
             break;
            }

            case 'i':    // i é uma operação de inserção
                ler_linha(buffer, COMP_REG, operacoesFile);
                strncpy(chave, buffer, 6); // mesma função do strcpy, mas esse da para escolher a quantidade de caracteres da string origem
                printf("\nINSERÇÃO DO REGISTRO DE CHAVE %s", chave);

                if (busca(chave, dados) != -1){  //significa que já existe uma chave naquela posição do FILE dados
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

            case 'r':  // r é uma operação de remoção

                ler_linha(chave, 7, operacoesFile);
                printf("\nREMOÇÃO DO REGISTRO %s\n", chave);
                rewind (dados);

                fread(&cabecalho, sizeof(cabecalho), 1, dados);

                RRN = busca(chave, dados);

                if(RRN == -1){
                    puts("\n------ERRO-----\n\nRegistro nao encontrado!\n");
                    break;
                }

                fputc('*', dados);
                fwrite(&cabecalho.topo_ped, sizeof(int), 1, dados);

                cabecalho.topo_ped = RRN;

                rewind(dados);
                fwrite(&cabecalho, sizeof(cabecalho), 1, dados);

                printf("REMOVIDO\n\n");
                printf("Posicao : RRN = %d (byte-offset %d)\n", RRN, byteoffset(RRN));
                break;

            default:
                printf("\nNÃO FOI POSSIVEL REMOVER");

                fseek(dados, 0, SEEK_END);
                if ((ftell(dados) - sizeof(cabecalho)) % COMP_REG != 0){   /* ftell = Retorna o valor atual do indicador de posição do fluxo. 
                                                                            Esse valor pode ser usado pela função fseek com origem SEEK_SET 
                                                                            para retornar o indicador a posição atual.*/
                    printf("\nNão foi possível realizar a operação\n");
                    exit(1);
                }
        }
    }

    fclose(operacoesFile);
    fclose(dados);
}

// vai imprimir os valores na PED que estão livres 
int imprime_ped(){                                          // ./programa -p 
    FILE *dados;
    dados = fopen("games.txt", "rb"); //mudar nome do arquivo
    int k, RRN;

    rewind (dados);
    if (dados == NULL){
        printf("\n\n----ERRO----\n");
        exit(1);
    }

    fread(&cabecalho, sizeof(cabecalho), 1, dados);
    RRN = cabecalho.topo_ped;

    printf("\nPED: %d", RRN);

    while (RRN > -1){
        RRN = PED(dados, RRN);
        printf(" = %d", RRN);
        k++;
    }
    fclose(dados);
    return 0;   
}

int main(int argc, char *argv[]){

    if(argc == 3 && strcmp(argv[1], "-i") == 0 ){
        printf("\n\nNome do arquivo que ocorreu importação: %s\n\n", argv[2]);
        importacao(argv[2]);

    } else if (argc == 3 && strcmp(argv[1], "-e") == 0 ){
        printf("\n\nModo de execução de operações ativado!! \n Nome do arquivo: %s\n", argv[2]);
        operacoes(argv[2]);

    } else if( argc == 2 && strcmp(argv[1], "-p") == 0 ){
        printf("\n\n--------Impressão da PED-------\n\n"); //mudar para LED
        imprime_ped();

    } else {
        fprintf(stderr, "Argumentos incorretos!\n\n");
        fprintf(stderr, "Modo de uso: \n");
        fprintf(stderr, "$ %s (-i|-e) nome_arquivo\n\n", argv[0]);
        fprintf(stderr, "$ %s -p\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

struct protofile { //структура, которая хранит параметры файла
    int nameSize;
    int fileSize;
    char *fileName;
    char *filePath;
};

int connectionClientBuilder (struct sockaddr_in addr) //устанавливает соединение через сокет со стороны клиента
{
    errno = 0;
    int cd;

    cd = socket(AF_INET, SOCK_STREAM, 0);
    if (cd < 0)
    {
        perror("socket");
        return __LINE__;
    }

    if ( connect(cd, (struct sockaddr *)&addr, sizeof (addr)) < 0)
    {
        perror("connect");
        return __LINE__;
    }

    return cd;
}

int connectionServerBuilder (struct sockaddr_in addr) //создает слушающий сокет на стороне сервера
{
    errno = 0;
    int sd;

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        perror("socket");
        return __LINE__;
    }

    if (bind(sd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("bind");
        return __LINE__;
    }

    listen(sd, 1);

    return sd;
}

int serverAcception(int sock) //принимает подключение от клиента
{
    int cd;

    cd = accept(sock, NULL, NULL);
    if (cd < 0)
    {
        perror("accept");
        return __LINE__;
    }

    return cd;
}

int getFilePath (struct protofile fileSet) //получает с консоли путь до файла, который будем передавать
{
    printf("Введите путь к файлу или наберите !end для завершения\n");
    gets(fileSet.filePath);

    if ( strcmp(fileSet.filePath, "!end") == 0 )
    {
        printf("Завершение программы...\n");
        return -1;
    }

    return 0;
}

void parser ( struct protofile fileSet ) //используется, чтобы получить имя файла без пути. Также нужен для принудительной записи в папку serverfiles
{
    int currentSlashPosition = 0;
    char currentChar;

    for (int i = 0; i <= strlen(fileSet.filePath); i++)
    {
        currentChar = fileSet.filePath[i];
        if (currentChar == '/') currentSlashPosition = i + 1;
    }

    int nameLength = strlen(fileSet.filePath) - currentSlashPosition;

    fileSet.fileName [nameLength];

    int j = 0;
    for (int i = currentSlashPosition; i <= strlen(fileSet.filePath); i++)
    {
        fileSet.fileName[j] = fileSet.filePath[i];
        j++;
    }

    printf("имя файла после парса: %s", fileSet.fileName);
}

int getNameSize (char* fileName) //вычисление размера имени
{
    int nameSize = strlen(fileName);
    return nameSize;
}

int getFileSize (FILE *file) //вычисление размера файла
{
    int fileSize;

    fseek(file, 0, SEEK_END);
    fileSize = (int) ftell(file);
    fseek(file, 0, SEEK_SET);

    return fileSize;
}

div_t getRounds (int fileSize) //вычисляет, сколько нужно отправить пакетов по 8 байт, чтобы отправить файл целиком
{
    div_t rounds;
    rounds = div(fileSize, 8);
    return rounds;
}

void fileAssetsSender (struct protofile fileSet, int sock) //отправляет параметры файла: размер имени и самого файла и его имя
{
    send(sock, &fileSet.nameSize, sizeof(int), 0);
    printf("размер имени: %d\n", fileSet.nameSize);
    send(sock, &fileSet.fileSize, sizeof(int), 0);
    send(sock, fileSet.fileName, fileSet.nameSize, 0);
}

int nameSizeReciever (int sock) //получает размер имени файла
{
    int nameSize = 0;
    recv(sock, &nameSize, sizeof(int), MSG_WAITALL);
    printf("размер имени: %d\n", nameSize);
    if (nameSize == 0) return 0;
    return nameSize;
}

int fileSizeReciever (int sock) //получает размер самого файла
{
    int fileSize = 0;
    recv(sock, &fileSize, sizeof(int), MSG_WAITALL);
    return fileSize;
}

char* fileNameReciever (int nameSize, char *fileName, int sock) //получает имя файла
{
    recv(sock, fileName, nameSize, MSG_WAITALL);
    return fileName;
}

void contentSender (FILE *file, struct protofile fileSet, int sock) //отправляет содержимое файла пакетами по 8 байт
{
    div_t rounds = getRounds(fileSet.fileSize);

    int *roundsNum = &rounds.quot;
    int *tail = &rounds.rem;

    send(sock, roundsNum, 4, 0); //отправляем сколько пакетов ждать
    send(sock, tail, 4, 0); //отправляем размер оставшегос "хвоста" байтов

    char buffer [8];
    for ( int i = 0; i <= *roundsNum; i++)
    {
        if (i == *roundsNum)
        {
            for (int j = 0; j < *tail; j++) buffer[j] = fgetc(file);

            send(sock, buffer, *tail, 0);
        }

        else
        {
            for ( int j = 0; j < 8; j++ ) buffer[j] = fgetc(file);

            send(sock, buffer, 8, 0);
        }
    }
}

void contentReciver (FILE *file, int sock) //получает содержимое файла пакетами по 8 байт
{
    int rounds, tail;
    int *roundsPoint = &rounds;
    int *tailPoint = &tail;

    recv(sock, roundsPoint, 4, MSG_WAITALL);
    recv(sock, tailPoint, 4, MSG_WAITALL);

    char buffer[8];
    for ( int i = 0; i <= rounds; i++ )
    {
        if ( i == rounds )
        {
            recv(sock, buffer, tail, MSG_WAITALL);
            for ( int j = 0; j < tail; j++ ) fputc( buffer[j], file );
        }
        else
        {
            recv(sock, buffer, 8, MSG_WAITALL);
            for ( int j = 0; j < 8; j++ ) fputc( buffer[j], file );
        }
    }
}

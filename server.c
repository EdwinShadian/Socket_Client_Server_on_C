#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "protocol.h"

int main()
{
    printf("Сервер запущен\n");

    errno = 0;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int sd = connectionServerBuilder(addr); //создаем слушающий сокет
    int cd = serverAcception(sd); //принимаем подключение от клиента

    printf("Соединение установлено, готов принять файл\n");

    for(;;) //сервер будет принимать файлы, пока клиент поддерживает соединение
    {
        struct protofile fileSet;

        fileSet.nameSize = nameSizeReciever(cd);
        if (fileSet.nameSize == 0) break;

        fileSet.fileSize = fileSizeReciever(cd);
        fileSet.fileName = calloc(fileSet.nameSize, sizeof(char *));

        strcpy(fileSet.fileName, fileNameReciever(fileSet.nameSize, fileSet.fileName, cd));

        printf("Приняты параметры файла %s размером %d байт\n", fileSet.fileName, fileSet.fileSize);
        printf("Принимаю файл %s\n", fileSet.fileName);

        fileSet.filePath = "./serverfiles/";

        char *buff = calloc(strlen(fileSet.fileName) + strlen(fileSet.filePath) + 1, sizeof (char)); //через буфер собираем полный путь, куда запишем файл

        strcat(buff, fileSet.filePath);
        strcat(buff, fileSet.fileName);

        fileSet.filePath = calloc(strlen(fileSet.fileName) + strlen(fileSet.filePath) + 1, sizeof (char));

        strcpy(fileSet.filePath, buff);

        free(buff);
        free(fileSet.fileName);

        FILE *file = fopen(fileSet.filePath, "w+");
        if ( file == NULL )
        {
            perror("file open");
            return __LINE__;
        }

        printf("Создан файл: %s\n", fileSet.filePath);

        free(fileSet.filePath);

        contentReciver(file, cd); //получаем содержимое файла

        fclose(file);

        printf("Готов принять следующий файл\n\n");
    }

    close(cd);
    close(sd);

    return EXIT_SUCCESS;
}

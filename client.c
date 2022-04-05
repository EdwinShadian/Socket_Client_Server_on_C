#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include "protocol.h"
#include <errno.h>

int main()
{
    printf("Клиент запущен\n");

    errno = 0;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int cd = connectionClientBuilder(addr); //устанавливаем соединение

    printf("Соединение установлено, готов отправлять файлы\n");
    for (;;) //клиент будет принимать файлы, пока не будет введено ключевое слово !end
    {
        struct protofile fileSet;

        fileSet.filePath = calloc(100, sizeof(char *));

        int result = 0;
        result = getFilePath(fileSet); //проверяем на соответствие ключевому слову

        if (result == -1) break;

        FILE *file = fopen(fileSet.filePath, "r");
        if ( file == NULL ) //если имя файла неверное - выводим сообщение об ошибке и ждем следующего файла
        {
            perror("file open");
            free(fileSet.filePath);
            continue;
        }

        fileSet.fileName = calloc(100, sizeof(char *));

        parser(fileSet);

        fileSet.nameSize = 0;
        fileSet.nameSize = getNameSize(fileSet.fileName);

        fileSet.fileSize = 0;
        fileSet.fileSize = getFileSize(file);

        fileAssetsSender(fileSet, cd); //отправляем параметры файла по протоколу

        printf("Отправлены параметры файла %s размером %d байт\n", fileSet.filePath, fileSet.fileSize);
        printf("Отправляю файл %s\n", fileSet.fileName);

        free(fileSet.filePath);

        contentSender(file, fileSet, cd); //отправляем содержимое файла по протоколу

        printf("Отправлен файл %s\n", fileSet.fileName);

        free(fileSet.fileName);

        fclose(file);

        printf("Готов отправить следующий файл\n\n");
    }

    close(cd);

    return EXIT_SUCCESS;
}

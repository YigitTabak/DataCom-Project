#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080            // Sunucunun �al��aca�� port numaras�
#define MAX_CLIENTS 2        // Ayn� anda ba�lanabilecek maksimum istemci say�s�
#define BUFFER_SIZE 1024     // Mesajlar i�in tampon boyutu

// Bu fonksiyon bir kelimenin, �nceki kelimenin son harfine uygun olup olmad���n� kontrol eder
int is_valid_word(const char *word, const char *last_word) {
    // E�er �nceki kelime bo� de�ilse ve yeni kelimenin ilk harfi,
    // �nceki kelimenin son harfiyle e�le�miyorsa ge�ersizdir.
    if (last_word[0] != '\0' && word[0] != last_word[strlen(last_word) - 1]) {
        return 0;
    }
    return 1; // Aksi takdirde ge�erli bir kelimedir.
}

int main() {
    int server_fd, new_socket, client_sockets[MAX_CLIENTS] = {0}; // Sunucu ve istemci soketleri
    struct sockaddr_in address; // Sunucunun adres bilgilerini tutar
    char buffer[BUFFER_SIZE];   // Mesajlar i�in tampon
    fd_set readfds;             // Soketlerdeki olaylar� kontrol etmek i�in kullan�l�r
    int max_sd, activity, i, valread; // Yard�mc� de�i�kenler
    socklen_t addrlen = sizeof(address);
    char last_word[BUFFER_SIZE] = ""; // Son ge�erli kelimeyi tutar

    // Sunucu soketi olu�turuluyor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed"); // Hata varsa bildir
        exit(EXIT_FAILURE);
    }

    // Sunucu adres yap�s�n� ayarla
    address.sin_family = AF_INET;        // IPv4 adres t�r�
    address.sin_addr.s_addr = INADDR_ANY; // Sunucunun t�m a� aray�zlerinden ba�lant� kabul etmesi
    address.sin_port = htons(PORT);     // Port numaras�n� a� bayt s�ras�na �evir

    // Sunucu soketini belirtilen adrese ba�la
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed"); // Hata varsa bildir
        exit(EXIT_FAILURE);
    }

    // Sunucu soketini dinleme moduna al
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed"); // Hata varsa bildir
        exit(EXIT_FAILURE);
    }

    printf("Server is running, waiting for connections...\n");

    // Ana d�ng�: istemcilerle ileti�imi y�netir
    while (1) {
        // T�m soketleri s�f�rla
        FD_ZERO(&readfds);

        // Sunucu soketini set'e ekle
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // Ba�lant�daki istemci soketlerini set'e ekle
        for (i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i]; // Her istemcinin soketini kontrol et
            if (sd > 0) FD_SET(sd, &readfds); // Ge�erli bir soket ise set'e ekle
            if (sd > max_sd) max_sd = sd;    // Maksimum soket de�erini g�ncelle
        }

        // Soketlerdeki aktiviteleri kontrol et
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("Select error"); // Hata varsa bildir
            exit(EXIT_FAILURE);
        }

        // Yeni bir ba�lant� talebi var m� kontrol et
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
                perror("Accept failed"); // Hata varsa bildir
                exit(EXIT_FAILURE);
            }

            printf("New client connected: socket %d\n", new_socket);

            // Yeni istemciyi ilk bo� alana yerle�tir
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket; // �stemciyi listeye ekle
                    break;
                }
            }
        }

        // Ba�l� istemcilerden gelen veriyi kontrol et
        for (i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];

            // �stemciden veri geldi�inde
            if (FD_ISSET(sd, &readfds)) {
                if ((valread = read(sd, buffer, BUFFER_SIZE)) == 0) {
                    // �stemci ba�lant�y� kapatt�
                    close(sd);
                    client_sockets[i] = 0; // �stemciyi listeden ��kar
                    printf("Client %d disconnected.\n", sd);
                } else {
                    buffer[valread] = '\0'; // Al�nan mesaj� sonland�r
                    printf("Client %d sent: %s\n", sd, buffer);

                    if (!is_valid_word(buffer, last_word)) {
                        // Ge�ersiz kelime -> Diskalifiye
                        printf("Client %d disqualified.\n", sd);
                        send(sd, "You are disqualified!\n", strlen("You are disqualified!\n"), 0);
                        close(sd);
                        client_sockets[i] = 0;

                        // Di�er istemciye kazand� mesaj� g�nder
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (client_sockets[j] != 0) {
                                send(client_sockets[j], "You win!\n", strlen("You win!\n"), 0);
                                printf("Client %d won the game.\n", client_sockets[j]);
                            }
                        }
                    } else {
                        // Ge�erli kelime ise g�ncelliyoruz
                        strcpy(last_word, buffer);

                        // Mesaj� yaln�zca di�er istemciye g�nder
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (client_sockets[j] != 0 && client_sockets[j] != sd) {
                                char msg[BUFFER_SIZE];
                                snprintf(msg, BUFFER_SIZE, "%s", buffer);
                                send(client_sockets[j], msg, strlen(msg), 0);
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}


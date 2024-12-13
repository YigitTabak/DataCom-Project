#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080            // Sunucunun çalýþacaðý port numarasý
#define MAX_CLIENTS 2        // Ayný anda baðlanabilecek maksimum istemci sayýsý
#define BUFFER_SIZE 1024     // Mesajlar için tampon boyutu

// Bu fonksiyon bir kelimenin, önceki kelimenin son harfine uygun olup olmadýðýný kontrol eder
int is_valid_word(const char *word, const char *last_word) {
    // Eðer önceki kelime boþ deðilse ve yeni kelimenin ilk harfi,
    // önceki kelimenin son harfiyle eþleþmiyorsa geçersizdir.
    if (last_word[0] != '\0' && word[0] != last_word[strlen(last_word) - 1]) {
        return 0;
    }
    return 1; // Aksi takdirde geçerli bir kelimedir.
}

int main() {
    int server_fd, new_socket, client_sockets[MAX_CLIENTS] = {0}; // Sunucu ve istemci soketleri
    struct sockaddr_in address; // Sunucunun adres bilgilerini tutar
    char buffer[BUFFER_SIZE];   // Mesajlar için tampon
    fd_set readfds;             // Soketlerdeki olaylarý kontrol etmek için kullanýlýr
    int max_sd, activity, i, valread; // Yardýmcý deðiþkenler
    socklen_t addrlen = sizeof(address);
    char last_word[BUFFER_SIZE] = ""; // Son geçerli kelimeyi tutar

    // Sunucu soketi oluþturuluyor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed"); // Hata varsa bildir
        exit(EXIT_FAILURE);
    }

    // Sunucu adres yapýsýný ayarla
    address.sin_family = AF_INET;        // IPv4 adres türü
    address.sin_addr.s_addr = INADDR_ANY; // Sunucunun tüm að arayüzlerinden baðlantý kabul etmesi
    address.sin_port = htons(PORT);     // Port numarasýný að bayt sýrasýna çevir

    // Sunucu soketini belirtilen adrese baðla
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

    // Ana döngü: istemcilerle iletiþimi yönetir
    while (1) {
        // Tüm soketleri sýfýrla
        FD_ZERO(&readfds);

        // Sunucu soketini set'e ekle
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // Baðlantýdaki istemci soketlerini set'e ekle
        for (i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i]; // Her istemcinin soketini kontrol et
            if (sd > 0) FD_SET(sd, &readfds); // Geçerli bir soket ise set'e ekle
            if (sd > max_sd) max_sd = sd;    // Maksimum soket deðerini güncelle
        }

        // Soketlerdeki aktiviteleri kontrol et
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("Select error"); // Hata varsa bildir
            exit(EXIT_FAILURE);
        }

        // Yeni bir baðlantý talebi var mý kontrol et
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
                perror("Accept failed"); // Hata varsa bildir
                exit(EXIT_FAILURE);
            }

            printf("New client connected: socket %d\n", new_socket);

            // Yeni istemciyi ilk boþ alana yerleþtir
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket; // Ýstemciyi listeye ekle
                    break;
                }
            }
        }

        // Baðlý istemcilerden gelen veriyi kontrol et
        for (i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];

            // Ýstemciden veri geldiðinde
            if (FD_ISSET(sd, &readfds)) {
                if ((valread = read(sd, buffer, BUFFER_SIZE)) == 0) {
                    // Ýstemci baðlantýyý kapattý
                    close(sd);
                    client_sockets[i] = 0; // Ýstemciyi listeden çýkar
                    printf("Client %d disconnected.\n", sd);
                } else {
                    buffer[valread] = '\0'; // Alýnan mesajý sonlandýr
                    printf("Client %d sent: %s\n", sd, buffer);

                    if (!is_valid_word(buffer, last_word)) {
                        // Geçersiz kelime -> Diskalifiye
                        printf("Client %d disqualified.\n", sd);
                        send(sd, "You are disqualified!\n", strlen("You are disqualified!\n"), 0);
                        close(sd);
                        client_sockets[i] = 0;

                        // Diðer istemciye kazandý mesajý gönder
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (client_sockets[j] != 0) {
                                send(client_sockets[j], "You win!\n", strlen("You win!\n"), 0);
                                printf("Client %d won the game.\n", client_sockets[j]);
                            }
                        }
                    } else {
                        // Geçerli kelime ise güncelliyoruz
                        strcpy(last_word, buffer);

                        // Mesajý yalnýzca diðer istemciye gönder
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


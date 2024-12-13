#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080           // Sunucunun baðlanacaðý port numarasý
#define BUFFER_SIZE 1024    // Mesaj tampon boyutu

int main() {
    int sock;                       // Client soket dosya tanýmlayýcýsý
    struct sockaddr_in server_address; // Sunucu adres bilgileri
    char buffer[BUFFER_SIZE];       // Sunucudan alýnan mesajlar için tampon
    char message[BUFFER_SIZE];      // Sunucuya gönderilecek mesajlar için tampon

    // Soket oluþturma
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error"); // Soket oluþturulurken hata varsa bildir
        return -1;
    }

    // Sunucu adres yapýsýný doldur
    server_address.sin_family = AF_INET;        // IPv4 adres ailesi
    server_address.sin_port = htons(PORT);     // Port numarasýný að bayt sýrasýna çevir

    // IP adresini binary formata çevir ve adres yapýsýna ekle
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported"); // IP adresi hatalýysa bildir
        return -1;
    }

    // Sunucuya baðlan
    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed"); // Baðlantý baþarýsýzsa bildir
        return -1;
    }

    printf("Connected to the server. Enter words to play:\n");

    // fd_set: Çoklu soketleri yönetmek için kullanýlan yapý
    fd_set readfds;

    // Oyun döngüsü: Kullanýcý giriþlerini ve sunucu mesajlarýný sürekli kontrol eder
    while (1) {
        FD_ZERO(&readfds);               // fd_set'i sýfýrla
        FD_SET(STDIN_FILENO, &readfds);  // Kullanýcý giriþini (standart giriþ) set'e ekle
        FD_SET(sock, &readfds);          // Sunucu soketini set'e ekle
        int max_sd = sock;               // Maksimum dosya tanýmlayýcýyý belirle

        // Aktif soketlerde deðiþiklik olup olmadýðýný kontrol et
        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("Select error"); // Select baþarýsýz olursa bildir
            exit(EXIT_FAILURE);
        }

        // Kullanýcýdan input alma 
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            fgets(message, BUFFER_SIZE, stdin);  // Client'ýn mesaj oku
            message[strcspn(message, "\n")] = '\0'; // Mesajýn sonunda yeni satýr karakterini kaldýr
            send(sock, message, strlen(message), 0); // Mesajý sunucuya gönder
        }

        // Sunucudan mesaj alýmý
        if (FD_ISSET(sock, &readfds)) {
            int valread = read(sock, buffer, BUFFER_SIZE); // Sunucudan mesaj oku
            if (valread > 0) {
                buffer[valread] = '\0'; // Gelen mesajý null terminator ile sonlandýr

                // Sunucudan Gelen mesajýn içeriðine göre iþlem yap
                if (strcmp(buffer, "You are disqualified!\n") == 0 || strcmp(buffer, "You win!\n") == 0) {
                    printf("%s", buffer); // Eðer mesaj diskalifiye ya da kazandý mesajýysa yazdýr
                } else {
                    printf("Message from another client: %s\n", buffer); // Diðer istemciden gelen mesaj
                }
            } else if (valread == 0) {
                // Eðer sunucu baðlantýyý kapattýysa
                printf("Server disconnected.\n");
                close(sock); // Soketi kapat
                break;       // Döngüden çýk
            }
        }
    }

    return 0;
}


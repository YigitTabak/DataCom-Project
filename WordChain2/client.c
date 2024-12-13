#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080           // Sunucunun ba�lanaca�� port numaras�
#define BUFFER_SIZE 1024    // Mesaj tampon boyutu

int main() {
    int sock;                       // Client soket dosya tan�mlay�c�s�
    struct sockaddr_in server_address; // Sunucu adres bilgileri
    char buffer[BUFFER_SIZE];       // Sunucudan al�nan mesajlar i�in tampon
    char message[BUFFER_SIZE];      // Sunucuya g�nderilecek mesajlar i�in tampon

    // Soket olu�turma
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error"); // Soket olu�turulurken hata varsa bildir
        return -1;
    }

    // Sunucu adres yap�s�n� doldur
    server_address.sin_family = AF_INET;        // IPv4 adres ailesi
    server_address.sin_port = htons(PORT);     // Port numaras�n� a� bayt s�ras�na �evir

    // IP adresini binary formata �evir ve adres yap�s�na ekle
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported"); // IP adresi hatal�ysa bildir
        return -1;
    }

    // Sunucuya ba�lan
    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed"); // Ba�lant� ba�ar�s�zsa bildir
        return -1;
    }

    printf("Connected to the server. Enter words to play:\n");

    // fd_set: �oklu soketleri y�netmek i�in kullan�lan yap�
    fd_set readfds;

    // Oyun d�ng�s�: Kullan�c� giri�lerini ve sunucu mesajlar�n� s�rekli kontrol eder
    while (1) {
        FD_ZERO(&readfds);               // fd_set'i s�f�rla
        FD_SET(STDIN_FILENO, &readfds);  // Kullan�c� giri�ini (standart giri�) set'e ekle
        FD_SET(sock, &readfds);          // Sunucu soketini set'e ekle
        int max_sd = sock;               // Maksimum dosya tan�mlay�c�y� belirle

        // Aktif soketlerde de�i�iklik olup olmad���n� kontrol et
        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("Select error"); // Select ba�ar�s�z olursa bildir
            exit(EXIT_FAILURE);
        }

        // Kullan�c�dan input alma 
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            fgets(message, BUFFER_SIZE, stdin);  // Client'�n mesaj oku
            message[strcspn(message, "\n")] = '\0'; // Mesaj�n sonunda yeni sat�r karakterini kald�r
            send(sock, message, strlen(message), 0); // Mesaj� sunucuya g�nder
        }

        // Sunucudan mesaj al�m�
        if (FD_ISSET(sock, &readfds)) {
            int valread = read(sock, buffer, BUFFER_SIZE); // Sunucudan mesaj oku
            if (valread > 0) {
                buffer[valread] = '\0'; // Gelen mesaj� null terminator ile sonland�r

                // Sunucudan Gelen mesaj�n i�eri�ine g�re i�lem yap
                if (strcmp(buffer, "You are disqualified!\n") == 0 || strcmp(buffer, "You win!\n") == 0) {
                    printf("%s", buffer); // E�er mesaj diskalifiye ya da kazand� mesaj�ysa yazd�r
                } else {
                    printf("Message from another client: %s\n", buffer); // Di�er istemciden gelen mesaj
                }
            } else if (valread == 0) {
                // E�er sunucu ba�lant�y� kapatt�ysa
                printf("Server disconnected.\n");
                close(sock); // Soketi kapat
                break;       // D�ng�den ��k
            }
        }
    }

    return 0;
}


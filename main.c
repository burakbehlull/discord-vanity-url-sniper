const char *guildToken = "sunucuları izlettirmek istediginiz hesabin tokeni";
const char *vanityToken = "sunucuya urlyi alıcak yetki verilmiş hesabın tokeni";
const char *serverIds[] = {"sunucu idleri"};
const char *webhookUrl = "webhook urlsi";
const char *vanities[] = {"istediginiz url"};
int heartbeatInterval = 0;
struct lws *socket = NULL;


int main() {
    printf("Merhaba snipper!");
    printf(guildToken);
    return 0;
}

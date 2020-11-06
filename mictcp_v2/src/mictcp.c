#include <mictcp.h>
#include <api/mictcp_core.h>

#define NUM_PORT_MAX 64512
#define TENTATIVE_MAX 10
//variable globale
mic_tcp_sock mysock;
mic_tcp_sock_addr addr_sock_dist;

int PE=0;
int PA=0;
int portsOccupe[NUM_PORT_MAX];
int nbOccuppe ;

/*

*/
int isOccuped(int port){
  for(int i=0;i<nbOccuppe;i++){
    if(portsOccupe[i]==port){
      return -1;
    }
  }
  return 0;
}
int random_port(){
  for(int i=0;i<NUM_PORT_MAX;i++){
    if(isOccuped(i+1024)==0){
      if(nbOccuppe>=NUM_PORT_MAX){
        return -1;
      }
      //add num port
      portsOccupe[nbOccuppe++] = i + 1024;
      return i + 1024;
    }
  }
  return -1;
}

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm)
{
   nbOccuppe = 0;

   int result = -1;
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   result = initialize_components(sm); /* Appel obligatoire */
   set_loss_rate(2);
   if(result!=-1){
     mysock.fd = 1;
     mysock.state = IDLE;
     return 1;
   }
   return result;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   if(mysock.fd ==socket ){
     // mysock.addr = addr;
     memcpy((char*)&mysock.addr,(char*)&addr,sizeof(mic_tcp_sock_addr));
     return 0;
   }
   return -1;
}
/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    if(mysock.fd ==socket ){
      mysock.state = CONNECTED;
      //addr_sock_dist = addr;
      return 0;
    }
    return -1;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    if(socket==mysock.fd){
      mysock.addr = addr;
      mysock.state = CONNECTED;
      int port = random_port();
      if(port==-1){
        printf("Erreur aucun numero de port est disponible\n");
        exit(EXIT_FAILURE);
      }
      mysock.port = port;
      //addr_sock_dist = addr;
      return 0;
    }
    return -1;
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */

 int mic_tcp_send (int socket, char* mesg, int mesg_size){
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   if((mysock.fd==socket) && mysock.state ==CONNECTED){
     mic_tcp_pdu pdu;
     mic_tcp_pdu ack;
     int taille_envoye =0;
     int nb_envois = 0;
     int timeout = 100; // 100ms;
     pdu.header.source_port = mysock.port;
     pdu.header.dest_port = mysock.addr.port ;

     pdu.payload.data = mesg;
     pdu.payload.size = mesg_size;

     pdu.header.syn = 0;
     pdu.header.ack = 0;
     pdu.header.fin = 0;
     pdu.header.seq_num = PE;
     PE = (PE +1) % 2;
     pdu.header.ack_num = 0;

     // Appel IPsend()
     taille_envoye = IP_send(pdu,mysock.addr);
     nb_envois++;
     // wait_for_ack
     mysock.state = WAIT_FOR_ACK;
     int ack_recu = 0;
     // Allocation memoire pour le ack
     ack.payload.size = 2 *(sizeof(unsigned short)) + 2 *(sizeof(int)) + 3 * (sizeof(unsigned char));
     ack.payload.data = malloc(ack.payload.size);
     while(ack_recu!=1){
       if((IP_recv(&ack,&mysock.addr,timeout)>=0) && (ack.header.ack==1) && (ack.header.ack_num==PE)){
         ack_recu = 1;
       }else{
         if(nb_envois<TENTATIVE_MAX){
            taille_envoye = IP_send(pdu,mysock.addr);
            nb_envois++;
         }else{
            printf("Nombre de renvoi max atteint (ERROR envoi paquet)\n");
            exit(EXIT_FAILURE);
          }
        }
    }
     mysock.state = CONNECTED;
     return taille_envoye;
   }
   return -1;
 }
/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    mic_tcp_payload p;
    int taille_lu = 0;
    p.data = mesg;
    p.size = max_mesg_size;

    if(mysock.fd==socket && mysock.state==CONNECTED){
      mysock.state = WAIT_FOR_PDU;
      taille_lu = app_buffer_get(p);
      mysock.state = CONNECTED;
    }
    return taille_lu;
}
/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
 int mic_tcp_close (int socket)
 {
     printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");

     if(close(socket)==0){
       return 0;
     }
     return -1;
 }
/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    mic_tcp_pdu ack;
    //printf("n°seq attentdu=%d , et n°seq recu =%d",PA,pdu.header.seq_num);
    if(pdu.header.seq_num==PA){
      //printf("Paquet Correct\n");
      PA = (PA + 1) % 2;
      app_buffer_put(pdu.payload);
    }
    // On rejet le PDU pas de stockage dans le buffer socklen
    // Envoi d'un ack avec num_ack PA
    ack.header.source_port = mysock.addr.port;
    ack.header.dest_port = addr.port;
    ack.header.ack = 1;
    ack.header.syn = 0;
    ack.header.fin = 0;
    ack.header.seq_num = 0;
    ack.header.ack_num = PA;

    ack.payload.size = 0;
    IP_send(ack,addr);
    //mysock.state = CONNECTED;
}

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
check if a port number is free
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
 * Allows to create a socket between the application and MIC-TCP
 * Returns the socket descriptor or -1 in case of error
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
 * Allows you to assign an address to a socket.
 * Returns 0 if successful, and -1 if unsuccessful.
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
 * Puts the socket in a connection acceptance state
 * Returns 0 if successful, -1 if error
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
 * Allows you to request a connection to be established
 * Returns 0 if the connection is established, and -1 if it fails.
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
 * Allows to request the sending of an application data
 * Returns the size of the data sent, and -1 in case of error
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
 * Allows the receiving application to request data recovery
 * stored in the socket's receive buffers
 * Returns the number of bytes read or -1 in case of error
 * NB: this function calls the app_buffer_get() function.
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
 * Allows to request the destruction of a socket.
 * Causes the connection to be closed according to the TCP model.
 * Returns 0 if all goes well and -1 in case of error.
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
 * Processing of a received MIC-TCP PDU (update of sequence numbers)
 * and acknowledgement, etc.) and then inserts the PDU user data in the
 * the socket's receive buffer. This function uses the
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

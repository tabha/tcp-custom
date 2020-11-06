#include <mictcp.h>
#include <api/mictcp_core.h>

#define MASQ 0x80
#define MASQ_P 0x7F
#define NUM_PORT_MAX 64512
#define TENTATIVE_MAX 10
#define PAQUETS 100
//variable globale
mic_tcp_sock mysock;
mic_tcp_sock_addr addr_sock_dist;

int PE=0;
int PA=0;
int portsOccupe[NUM_PORT_MAX];
int nbOccuppe ;
int paquestRecu[PAQUETS]; //1 recu 0 perdu
int nextIndexMessage = 0;
float perteAutorisee = 0.0;
int sequence_init=0;
unsigned long timerServer = 0.0;
unsigned long timeoutServer = 10;
int srv_sequence_init_clt;
int srv_nb_envois = 0;

/*
* returns the number of lost packets in the packetsRecu
*/
int getNombrePaquetsPerdu(){
  int count = 0;
  for(int i = 0;i<PAQUETS;i++){
    if(paquestRecu[i]==0){
      count++;
    }
  }
  return count;
}

/*
* calculates the percentage of lost packets
*/
float getPourcentagePerte(){
  float perte = 0.0;
  int paquetsPerdu = getNombrePaquetsPerdu();
  printf("%d paquets Perdu \n",paquetsPerdu);
  perte = ((float)paquetsPerdu) / (float)PAQUETS;
  return perte;
}
/*
* check if a port number is free
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
   if(sm==CLIENT){
     sequence_init = 50;
     set_loss_rate(0);

   }else{
     sequence_init = 20;
     set_loss_rate(1);
   }
   int result = -1;
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   result = initialize_components(sm); /* Appel obligatoire */
   for(int i=0;i<PAQUETS;i++){
     paquestRecu[i] = 1;
   }
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
      mysock.state = WAIT_FOR_SYN;
      //printf("Wait for syn\n");
      while(mysock.state != ESTABLISHED){}
      //printf("ESTABLISHED\n");
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

      unsigned char syn_h =2;
      mic_tcp_pdu syn,syn_ack,ack;
      mysock.addr = addr;
      int nb_essais = 0;
      float timer = 100.0;
      int port = random_port();
      if(port==-1){
        printf("Erreur aucun numero de port est disponible\n");
        exit(EXIT_FAILURE);
      }
      // syn 1000 0001 le 7 LSB code le pourccentage de perte le MSB designe si oui ou non c'est un syn
      syn_h = syn_h | (1<<7); // mise à 1 du MSB
      mysock.port = port;
      // Construction du syn avec n°seq = sequence_init
      syn.header.source_port = port;
      syn.header.dest_port = mysock.addr.port;
      syn.header.ack = 0;
      syn.header.syn = syn_h;
      syn.header.seq_num = sequence_init;
      syn.header.ack_num = 0;
      syn.payload.size = 0;
      // Envoi du pdu syn et se mettre en attente
      if(IP_send(syn,mysock.addr)<0){
        printf("Erreur envoi du paquet de synchronisation\n");
        exit(EXIT_FAILURE);
      }
      nb_essais++;
      mysock.state = SYN_SENT;
      syn_ack.payload.size = 2 *(sizeof(unsigned short)) + 2 *(sizeof(int)) + 3 * (sizeof(unsigned char));
      syn_ack.payload.data = malloc(syn_ack.payload.size);
      // Attente du syn_ack du serveur et Traitement de ce dernier.
      while(mysock.state!=ACK_RECEIVED){ // ce ack contient egalement le numero de sequence du serveur.
        if((IP_recv(&syn_ack,&mysock.addr,timer)>=0)&& (syn_ack.header.ack==1)&&(syn_ack.header.syn==1)&&(syn_ack.header.ack_num==sequence_init+1)){
          mysock.state = ACK_RECEIVED;
        }else{
          //printf("sequence attendu (ack=1,syn=1,num_seq=%d )  VS sequence recu (ack=%d,syn=%d,num_seq=%d )\n",sequence_init+1,syn_ack.header.ack,syn_ack.header.syn,syn_ack.header.ack_num);
          if(nb_essais < TENTATIVE_MAX){
            if(IP_send(syn,mysock.addr)<0){
              printf("Erreur envoi du paquet de synchronisation\n");
              exit(EXIT_FAILURE);
            }
            nb_essais++;
          }else{
            printf("Nombre de te tentative max atteint (Le serveur ne repond pas)\n");
            exit(EXIT_FAILURE);
          }
        }
      }
      int num_seqServ = syn_ack.header.seq_num; // utile pour repondre au serveur

      // construction du ack pour la dernière étape.
      ack.header.source_port = port;
      ack.header.dest_port = mysock.addr.port;
      ack.header.ack = 1;
      ack.header.syn = 0;
      ack.header.seq_num = sequence_init+1;
      ack.header.ack_num = num_seqServ + 1;
      ack.payload.size = 0;
      if(IP_send(ack,mysock.addr)<0){
        printf("Erreur envoi du paquet de synchronisation\n");
        exit(EXIT_FAILURE);
      }
      mysock.state = CONNECTED;
      return 0;
    }
    return -1;
}

/*
 * Allows to request the sending of an application data
 * Returns the size of the data sent, and -1 in case of error
 */

 int mic_tcp_send (int socket, char* mesg, int mesg_size){
   //printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
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
     ack.header.ack_num = PE;
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
         if(nb_envois<TENTATIVE_MAX){ //
            printf("Retransmission du paquet n°%d\n",ack.header.ack_num);
            taille_envoye = IP_send(pdu,mysock.addr);
            nb_envois++;
         }
         else{
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
      //mysock.state = WAIT_FOR_PDU;
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
    //printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    mic_tcp_pdu ack;
    //printf("n°seq attentdu=%d , et n°seq recu =%d",PA,pdu.header.seq_num);

    if(mysock.state ==CONNECTED){
      if(pdu.header.seq_num==PA){
        //printf("Paquet Correct\n");
        paquestRecu[nextIndexMessage] = 1;
        nextIndexMessage = (nextIndexMessage + 1) % PAQUETS;
        //printf("Next Index Message %d\n",nextIndexMessage);
        PA = (PA + 1) % 2;
        app_buffer_put(pdu.payload);
      }else{
        float perte = getPourcentagePerte();
        printf("Pourcentage Pertes=%f VS Pertes Autorisee %f\n",perte,perteAutorisee);
        //printf("Next Index Message %d\n",nextIndexMessage);
        if(perte <=perteAutorisee){
          printf("Paquet Perdu et Non retransmis\n");
          paquestRecu[nextIndexMessage] = 0;
          nextIndexMessage = (nextIndexMessage + 1) % PAQUETS;
          PA = (PA+1)%2;
        }else{
          printf("Demande de retransmission du paquet n°%d\n",PA);
        }
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
      return (void)NULL;
    }
    if(mysock.state == WAIT_FOR_SYN){
      // ON teste que le MSB est à 1.
      if((pdu.header.syn & MASQ)!=0){ // c'est un paquet de synchronisation
        //printf("Envoi de syn\n");
        perteAutorisee =  (float) (pdu.header.syn & ~(1<<7))/ 100.0;
        printf("Pourcentage de perte est de %f %% \n",perteAutorisee);
        mic_tcp_pdu syn_ack;
        srv_sequence_init_clt = pdu.header.seq_num;
        syn_ack.header.source_port = mysock.addr.port;
        syn_ack.header.dest_port = addr.port;
        syn_ack.header.syn = 1;
        syn_ack.header.ack = 1;
        syn_ack.header.seq_num = sequence_init;
        syn_ack.header.ack_num = srv_sequence_init_clt + 1;
        syn_ack.payload.size = 0;
        timerServer = get_now_time_msec();
        if(IP_send(syn_ack,mysock.addr)<0){
          //printf("Erreur envoi du paquet de synchronisation (syn_ack)\n");
          exit(EXIT_FAILURE);
        }
        // activate timer;
        mysock.state = WAIT_FOR_ACK;
      }
      //printf("Paquet incorrect\n");
      return (void)NULL;
    }
    if(mysock.state==WAIT_FOR_ACK){//
      if(pdu.header.ack && (pdu.header.seq_num==srv_sequence_init_clt + 1) && (pdu.header.ack_num==sequence_init+1) ){
          mysock.state = ESTABLISHED;
      }else{
        unsigned long currentDate = get_now_time_msec();
        if((currentDate-timerServer)>timeoutServer){
          timerServer = 0;
          perteAutorisee = 0.0;
          mysock.state = WAIT_FOR_SYN;
        }
      }
      return (void) NULL;
    }

}

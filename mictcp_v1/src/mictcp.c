#include <mictcp.h>
#include <api/mictcp_core.h>

//variable globale
mic_tcp_sock mysock;
mic_tcp_sock_addr addr_sock_dist;

/*
 * Allows to create a socket between the application and MIC-TCP
 * Returns the socket descriptor or -1 in case of error
 */
int mic_tcp_socket(start_mode sm)
{
   int result = -1;
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   result = initialize_components(sm); /* Appel obligatoire */
   //set_loss_rate(0);
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
      return 0;
    }
    return -1;
}

/*
 * Allows to request the sending of an application data
 * Returns the size of the data sent, and -1 in case of error
 */

 int mic_tcp_send (int socket, char* mesg, int mesg_size){
   if((mysock.fd==socket) && mysock.state ==CONNECTED){
     mic_tcp_pdu pdu;
     int taille_envoye =0;
     pdu.header.source_port = mysock.addr.port;
     pdu.header.dest_port = addr_sock_dist.port ;

     pdu.payload.data = mesg;
     pdu.payload.size = mesg_size;

     pdu.header.syn = 0;
     pdu.header.ack = 0;
     pdu.header.fin = 0;
     pdu.header.seq_num = 0;
     pdu.header.ack_num = 0;

     // Appel IPsend()
     taille_envoye = IP_send(pdu,mysock.addr);
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
    p.data = mesg;
    p.size = max_mesg_size;

    int taille_lu = app_buffer_get(p);
    return taille_lu;
}
/*
 *  Allows to request the destruction of a socket.
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
    app_buffer_put(pdu.payload);
    mysock.state = CONNECTED;
}

namespace Manch{
  
  #define VERBOSE 0 //change to 1 to print everything
  
  char status;
  enum {
    PREAMBLE, //waiting for checksum
    SYNC,     //synced signal
    CHECK,    //passed checksum
    RESYNC    //lost sync
  };
  

  
  /** Called every falling or rising edge */
  void dec(bool s, uint16_t t){
    
    static uint16_t T; //timing of a half bit
    
    static uint8_t i; //count edges on the preamble stage
    
    static bool w; //keep track of the 2T timing
     
    static struct { //used to store bits until a full byte is received
        byte buffer;
        uint8_t pos;
    } bits;
    
    ++i;
    
    if(status==RESYNC){
        //reset everything and wait for a preamble and resync
        i=0;
        T=0;
        w=0;
        
        bits.buffer=0x0;
        bits.pos=0;
        
        status=PREAMBLE;
        
        #if VERBOSE==1
         Serial.print("<RESYNC> ");
        #endif
    }
    
    //preamble
    if(status==PREAMBLE){
      
        #if VERBOSE==1
          Serial.println();
          Serial.print(s);
          Serial.print(" ");
          Serial.print(t);
          Serial.print(" preamble");  
        #endif
        
        if(i>3){
            
            if(!T){ //undefined T
              T = t;
              
              #if VERBOSE==1
                Serial.print(" t=");
                Serial.print(T);
              #endif
            }
            else if(t<T*0.75){ //found a much smaller T
                T=t;
                
                #if VERBOSE==1
                Serial.print(" smaller t=");
                Serial.print(T);
              #endif
            }
            else if(t>T*0.75 && t<=T*1.5){ //just T
            
            }
            else if(t>T*1.5 && t<T*2.5 ){ //first 2T is a sync signal
                   status = SYNC;
                   i = 0;
                   
                   bits.pos=4; //checksum start at the 4th bit
                   bits.buffer = bits.buffer | s<<7-bits.pos++;
                   
                   #if VERBOSE==1
                     Serial.println(" SYNC!\n");
                   #endif
            }
            else{ //found much longer T
              status = RESYNC;
            }
        }
  
    }
    
    else{ // SYNCED start to decode to buffer
      
      if(t>T*2.5 || t<T*0.75){ //timming is very off
        status=RESYNC;
      }
      
      //decode
      if(t>T*1.5){ //2t
        bits.buffer = bits.buffer | s<<7-bits.pos++; 
      }
      else{ //t
         if(w<1){ //first t
           w=1;
         }
         else{ //second t
           bits.buffer = bits.buffer | s<<7-bits.pos++;
           w=0;
         }        
      }
      
      
      //do a CHECKSUM after sync signal
      if(status==SYNC){
        
        if(bits.pos>7){// wait for a complete byte  
           if(bits.buffer==0b0100){ //checksum
             status = CHECK;
             
             #if VERBOSE==1
               Serial.println(" CHECK! \n");
             #endif
             
             //clear
             bits.buffer=0x0;
             bits.pos=0;
           }
           else{ //invalid checksum
             status=RESYNC;
             
             #if VERBOSE==1
               Serial.println("<INVALID CHECKSUM>");
             #endif
             
             
          }
        } 
      }
      
      
      //start pushing bytes to out buffer
      else if(status==CHECK){
        //received a full byte
        if(bits.pos>7){
            
            //Serial.println();
            //Serial.println(buffer, BIN);
            //Serial.println(b, DEC);
            //Serial.write(b);
            
            Buffer::enqueue(bits.buffer);
            
            bits.buffer = 0x0; //clear
            bits.pos = 0;
        }
      }
      
      
    }
  
  }
  
  uint8_t available(){
      return Buffer::queuelevel();
  }
  byte read(){
      return Buffer::dequeue();
  }

}
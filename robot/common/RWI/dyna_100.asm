; Dynamixel accelerator for reading multiple AX-12 servos quickly

; receives a multi-read command and range of servo addresses
; sends out requests sequentially and waits for each return
; snooping on bus lets USB converter pick up whole set as one packet
;
;   USB  -> FF FF 7F 04 : 84 01 03 ch              (cmd to PIC @ 7F)
;
;   PIC  -> FF FF 01 04 : 02 24 06 ch              (read from servo 1)
;   AX12 -> FF FF 01 08 : ee p0 p1 v0 v1 f0 f1 ch  (pos, vel, and force)
;   PIC  -> FF FF 02 04 : 02 24 06 ch              (read from servo 2)
;   AX12 -> FF FF 02 08 : ee p0 p1 v0 v1 f0 f1 ch  (pos, vel, and force)
;   PIC  -> FF FF 03 04 : 02 24 06 ch              (read from servo 3)
;   AX12 -> FF FF 03 08 : ee p0 p1 v0 v1 f0 f1 ch  (pos, vel, and force)
;
; size of response is 20N bytes (e.g. 200 for 8 arm + 2 neck servos)
; expects Dynamixel to run at 1Mbaud (16MHz ext osc / 16 for PIC)

; ------------------------------------------------------------------

; Written by Jonathan H. Connell, jconnell@alum.mit.edu
;
; /////////////////////////////////////////////////////////////////////////
;
; Copyright 2012 IBM Corporation
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;    http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.
; 
; /////////////////////////////////////////////////////////////////////////

; Versions:
;
;   1.00 - reads position, velocity, and force from servos ID0-IDN


; ==================================================================

  radix dec		  ; numbers default to base 10
  list p=16f688           ; for list file instruction mnemonics
  #include "p16f688.inc"  ; register and bit definitions
 

  __CONFIG 0x03CA	  ; brown, no prot, A3, delay, watch, HS osc
			  ; pattern = 0011 1100 1010
			  ;    bits = BA98 7654 3210


; ------------------------------------------------------------------
; port bits 

LED	EQU	2	; RC2 = LED indicator output (pin 8)
TXPIN	EQU	3	; RC3 = tri-state enable TX output (pin 7)


; ------------------------------------------------------------------
; operating constants

ME	EQU	0x7F	; Dynamixel address of this PIC
PVF	EQU	0x84	; mega-read command from USB converter
ADDR	EQU	0x24	; AX-12 register to start reading
NUM	EQU	6	; number of bytes to read from AX-12
DUMP	EQU	12	; total number of bytes in AX-12 return 


; ------------------------------------------------------------------
; RAM register allocation

ID0	EQU	0x20	; next servo to read from 
IDN	EQU	0x21	; last servo to read from
CNT	EQU	0x22	; counter
CHK	EQU	0x23	; checksum
TMP	EQU	0x24	; temporary


; ==================================================================
; program section

  ORG 0x000			; >> boot location  
  nop				; for optional debugger
  goto start


  ORG 0x004			; >> interrupt location
  retfie


; ------------------------------------------------------------------
; main loop, no interrupt processing needed

start
  call	sys_init		; set up serial, port bits, etc.

main
  call  mega_rd			; wait for request
prompt
  call	request			; ask for servo N's data
  movlw	DUMP			; wait for return packet (sync + ID + data)
  call	drain
  incf	ID0,F			; advance to next number
  movf	ID0,W			; see if all servos read yet
  subwf	IDN,W
  btfsc	STATUS,C
  goto  prompt
  goto  main
	

; ------------------------------------------------------------------

; wait for a fully formed multi-read request (saves ID0 and IDN)
; looks for correctly formed packet and checksum, resyncs if needed

mega_rd
  btfsc	RCSTA,OERR		; clear any overrun error
  bcf	RCSTA,CREN
  bsf	RCSTA,CREN		; make sure receiver is enabled
sync
  clrwdt			; prevent timeout (just comes back)
  call  rx			; read first sync byte
  sublw 0xFF
  btfss	STATUS,Z
  goto	sync
  clrwdt			; setup for timeout
  call 	rx			; read second sync byte
  sublw 0xFF
  btfss	STATUS,Z
  goto	sync
chk_id
  clrf	CHK			; clear checksum
  call 	rx			; see if packet for this PIC
  sublw	ME
  btfsc	STATUS,Z
  goto 	chk_len
  call	rx			; get rid of rest of bad packet
  call	drain			
  goto	sync
chk_len
  call	rx			; save packet length and check
  movwf	CNT
  sublw 4
  btfsc	STATUS,Z
  goto	chk_cmd
  movf	CNT,W			; get rid of rest of bad packet
  call	drain
  goto	sync
chk_cmd
  call	rx			; see if proper mega-read command 
  sublw	PVF
  btfsc	STATUS,Z
  goto	get_args
  decf	CNT,W			; get rid of rest of bad packet
  call 	drain
  goto	sync
get_args
  call	rx			; save starting servo number
  movwf	ID0
  call	rx			; save ending servo number
  movwf	IDN
  comf	CHK,W			; save checksum so far (inverted)
  movwf	TMP
  call	rx			; see if checksum matches
  subwf	TMP,W
  btfss	STATUS,Z
  goto	sync
  return


; carefully read a byte from receiver to W and add to checksum

rx
  btfss PIR1,RCIF		; wait for a byte
  goto 	rx
  btfss	RCSTA,FERR		; check for framing error
  goto	rx_ok
  movf	RCREG,W			; remove bad byte and try again
  goto	rx
rx_ok
  movf	RCREG,W			; get byte into accumulator
  addwf	CHK,F			; add into checksum
  return


; wait for number of bytes in W to be received (no error checking)
; potentially times out and PIC resets if bad bytes or too few received

drain
  clrwdt			; setup for timeout
  bsf	PORTC,LED
  movwf	CNT			; initialize count
rx_any
  btfss PIR1,RCIF		; wait for a byte
  goto 	rx_any
  movf	RCREG,W			; clear receive flag and ignore byte
  decfsz CNT,F			; see if done yet
  goto 	rx_any
  bcf	PORTC,LED
  return


; request position, velocity, and force from some servo
; servo to read is passed in ID0 register

request
  clrwdt			; prevent accidental timeout
  bcf	RCSTA,CREN		; don't listen to self
  movlw	13			; wait 10.25us (= 0.5 + 13 * 0.75us)
  movwf	CNT
tx_wait
  decfsz CNT,F
  goto	tx_wait
  bsf	PORTC,TXPIN		; enable tri-state buffer
  movlw 0xFF			; send packet sync bytes
  call	tx
  movlw 0xFF			
  call  tx
  clrf	CHK			; clear checksum
  movf	ID0,W			; send servo ID
  call  tx
  movlw 4			; command length (always 4)
  call  tx
  movlw 0x02			; read command (defined as 2)
  call 	tx
  movlw	ADDR			; where to read from
  call 	tx
  movlw	NUM			; how much to read
  call 	tx
  comf	CHK,W			; send checksum (inverted)
  call	tx
tx_end
  btfss	TXSTA,TRMT		; wait for last byte to finish
  goto	tx_end
  bcf	PORTC,TXPIN		; turn transmitter off
  bsf	RCSTA,CREN		; start listening again
  return


; send byte in W to transmitter 
; also adds to ongoing checksum AFTER transmission

tx
  btfss	PIR1,TXIF		; wait for buffer to be ready
  goto 	tx
  movwf	TXREG			; queue for transmission
  addwf	CHK,F			; update checksum
  return


; ------------------------------------------------------------------
; initial setup of ports, timers, A/D converter, etc.
; watchdog using 32Khz with lowest prescaler (1/32) = 1ms timeout

sys_init
  clrf	PORTC			; make sure all outputs low
  clrf	WDTCON			; watchdog at 1ms
  bsf	TXSTA,BRGH		; serial at Fosc/16 = 1Mbaud
  bsf	TXSTA,TXEN		; enable serial transmitter
  bsf	RCSTA,CREN		; enable serial receiver
  bsf	RCSTA,SPEN		; enable serial communication
  bsf   STATUS,RP0		; -- switch to register bank 1
  bcf	OPTION_REG,PSA		; prescaler assigned to timer0
  bcf	TRISC,TXPIN		; tri-state enable is output
  bcf	TRISC,LED		; signal LED is output
  bcf 	STATUS,RP0		; -- switch back to register bank 0
  return


  END
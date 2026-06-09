; Dynamixel accelerator for reading multiple AX-12 servos quickly

; unique ID coded must be changed manually using the "ROBOT" constant
; for unique ID processor sends out one byte of garbage (AX-12 ignores)
;
;   USB  => FF FF 7F 02 - 86 : ch                    (cmd to PIC @ 7F)
;   PIC  <- FF FF 7F 02 - id : ch                    (unique ID number)   
;
; receives a multi-read command and range of servo addresses
; sends out requests sequentially and waits for each return
; snooping on bus lets USB converter pick up whole set as one packet
;
;   USB  => FF FF 7F 04 - 84 01 03 : ch              (cmd to PIC @ 7F, accel = 85)
;
;   PIC  <- FF FF 7F 05 - xp yp x0 y0 : ch           (optional accelerometers if 85)
;
;   PIC   * FF FF 01 04 - 02 24 06 : ch              (request to servo 1)
;   AX12 <- FF FF 01 08 - ee p0 p1 v0 v1 f0 f1 : ch  (pos, vel, and force)
;   PIC   * FF FF 02 04 - 02 24 06 : ch              (request to servo 2)
;   AX12 <- FF FF 02 08 - ee p0 p1 v0 v1 f0 f1 : ch  (pos, vel, and force)
;   PIC   * FF FF 03 04 - 02 24 06 : ch              (request to servo 3)
;   AX12 <- FF FF 03 08 - ee p0 p1 v0 v1 f0 f1 : ch  (pos, vel, and force)
;
; size of response is 20N bytes (e.g. 200 for 8 arm + 2 neck servos) + 9 (if accel)
; expects Dynamixel to run at 1Mbaud (16MHz ext osc / 16 for PIC)
;
; moderate shock = 0.75G: 1.68V --7ms--> +600mv --5ms--> -300mv
;   3.3v Vref @ 8 bits: -1.5G = 0.45V (35), +1.5G = 2.86V (221), so 16.18mG/bit
; sample each channel @ 2 KHz, change channel then convert (4 phases)
;   interrupt must be less than about 2 x 10 bits * 1us = 20us (80 instros)
; AN0 is X channel = back/forward (pin 13)
; AN2 is Y channel = right/left   (pin 11)
; AN4 is Z channel = up/down      (pin 10) - not used

; written by Jonathan Connell 
; Copyright 2012-2016, IBM, All Rights Reserved


; ------------------------------------------------------------------
; Versions:
;
;   1.00 - reads position, velocity, and force from servos ID0-IDN
;   1.01 - reports unique body number (8 bit) for cmd 0x85
;   1.02 - explicit timeout on RX (no watchdog) change id cmd 0x86
;   1.03 - scan XYZ accelerometers at 2.6KHz and give 1.25x values
;   1.04 - scan XY accelerometers at 2KHz and give 2x averages & peaks


; ==================================================================
; =                           DEFINITIONS                          =
; ==================================================================

  radix dec		  ; numbers default to base 10
  list p=16f688           ; for list file instruction mnemonics
  #include "p16f688.inc"  ; register and bit definitions
 
  __CONFIG 0x03C2	  ; brown, no prot, A3, delay, no watch, HS osc
			  ; pattern = 0011 1100 0010
			  ;    bits = BA98 7654 3210


; ------------------------------------------------------------------
; operating constants

ROBOT   EQU     2	; serial number of this robot <== CHANGE <==

ME	EQU	0x7F	; Dynamixel address of the PIC
PVF	EQU	0x84	; mega-read command from USB converter
WHO     EQU     0x86    ; ID read command from USB converter
ADDR	EQU	0x24	; AX-12 register to start reading
NUM	EQU	6	; number of bytes to read from AX-12
DUMP	EQU	12	; total number of bytes in AX-12 return 
RXMAX	EQU	128	; receiver timeout (x 128us -> 16ms)

XFER	EQU	7	; FLAGS bit - record new peak found
SKIP	EQU	6	; FLAGS bit - do not add values to sum
AGET	EQU	0	; FLAGS bit - harvest A/D value3
ATOL	EQU	25	; deviation for averaging (10% = 0.4G)
GAP	EQU	64	; number of skipped averages to allow
ACMAX	EQU	128	; accelerometer timeout (x 512us -> 66ms)


; ------------------------------------------------------------------
; port bits 

LED	EQU	2	; RC2 = LED indicator output (pin 8)
TXPIN	EQU	3	; RC3 = tri-state enable TX output (pin 7)


; ------------------------------------------------------------------
; RAM register allocation


; saved registers during interrupt

W_SAVE	EQU	0x20	; working register
S_SAVE	EQU	0x21	; status register


; main loop serial packets

ID0	EQU	0x22	; next servo to read from 
IDN	EQU	0x23	; last servo to read from
CNT	EQU	0x24	; command byte count
CMD	EQU	0x25	; temporarily saved packet command
TST	EQU	0x26	; temporarily saved packet checksum
CHK	EQU	0x27	; computed checksum
RXTIM	EQU	0x28	; receiver timeout counter

XPK2	EQU	0x29	; X during peak shock (tx copy)
YPK2	EQU	0x2A	; Y during peak shock (tx copy)


; interrupt routine accelerometer

FLAGS	EQU	0x2B    ; accelerometer control flags
XVAL	EQU	0x2C	; recent X accelerometer 
YVAL	EQU	0x2D	; recent Y accelerometer 
DIFF	EQU	0x2E	; temporary unisgned difference
PEAK 	EQU	0x2F	; unsigned max over channels
NSUM	EQU	0x30	; sum samples needed (counts down)
MISS	EQU	0x31	; bad samples tolerated (counts down)
ACTIM	EQU	0x32	; samples since peak reset

XHI	EQU	0x33    ; current sum of X readings (MSB)
XLO	EQU	0x34	; current sum of X readings (LSB)
YHI	EQU	0x35    ; current sum of Y readings (MSB)
YLO	EQU	0x36	; current sum of Y readings (LSB)
XAVG	EQU	0x37	; recent X average (16.18 mG/bit)
YAVG	EQU	0x38	; recent Y average (16.18 mG/bit)
XAV4	EQU	0x39	; recent X average (4.05 mG/bit)
YAV4	EQU	0x3A	; recent Y average (4.05 mG/bit)
XPK	EQU	0x3B	; X during peak shock 
YPK	EQU	0x3C	; Y during peak shock


; ==================================================================
; =                       PROGRAM SECTION                          =
; ==================================================================

  ORG 0x000			; >> boot location  

  nop				; for optional debugger
  goto start


; ==================================================================

; background interrupt samples accelerometer and checks timeout
; saves current readings in XVAL and YVAL over 4 interrupt cycles
;   one phase starts conversion, other harvests value and selects new channel
; if close to XAVG and YAVG then adds to sums in XHI:LO and YHI:YLO
;   after 64 samples, transfers to XAVG, XAV4, YAVG, YAV4 and zeroes
;   if a long time without valid then sets XAVG and YAVG to defaults
; if abs(X-XAVG) or abs(Y-YAVG) >= PEAK then saves readings in XPK and YPK
;   resets peaks after transmission or if timeout 
; phase number consists of ADCON0<CHS1> : FLAGS<AGET>
; worst case cycle counts by phase (20ms allowed):
;   00: x0 cvt = 11 + (5 + 5 + 28)      + 7 = 56 (14.00ms)
;   01: x1 get = 11 + (5 + 21)          + 7 = 44 (11.00ms)
;   10: y2 cvt = 11 + (5 + 30)          + 7 = 53 (13.25ms)
;   11: y3 get = 11 + (5 + 21 + 7 + 14) + 7 = 65 (16.25ms)


  ORG 0x004			; >> interrupt location


; ------ SAVE & DISPATCH ------ (11)

  bsf	PORTC,LED		; signal interrupt start
  bcf	INTCON,T0IF		; clear timer0 overflow
  movwf	W_SAVE			; backup W and STATUS registers
  swapf	STATUS,W
  movwf	S_SAVE
  movf	RXTIM,F			; see if timeout engaged
  btfss	STATUS,Z
  decf	RXTIM,F			; decrement timer down to zero
  btfsc	FLAGS,AGET		; see if analog selection stable
  goto	harvest


; ------ PHASEs 0 & 2 ------ (5)

convert
  bsf	FLAGS,AGET		; harvest value on next cycle
  bsf	ADCON0,GO		; start conversion of pre-selected channel
  btfsc ADCON0,CHS1		; see if AN0 or AN2 being converted		
  goto 	ych_2
 

; ------ (5 + 28)


pk_reset
  decfsz ACTIM,F		; see if too long since peaks read
  goto	xch_0
  clrf	PEAK			; erase old peak value threshold
  movlw	ACMAX                   ; restart interval
  movwf	ACTIM

xch_0			
  movf	NSUM,W			; see if enough samples in X sum
  btfss	STATUS,Z
  goto	int_done
norm_x
  rlf	XLO,F			; shift 16 bit value up by 2
  rlf	XHI,F
  rlf	XLO,F
  rlf	XHI,F
  movf	XHI,W			; save as low precision average
  movwf	XAVG
  clrf	XAV4			; default x4 value is zero
  movlw	96			; check if X value too low
  subwf	XHI,F
  btfss STATUS,C
  goto	nx_done
scale_x
  movlw	0xFF			; default x4 value is now 255
  rlf	XLO,F			; scale up resolution by 4
  rlf	XHI,F
  btfsc	STATUS,C		; see if too high
  goto	nx_done
  rlf	XLO,F
  rlf	XHI,F
  btfss	STATUS,C		; see if too high
  movf	XHI,W
nx_done
  movwf	XAV4			; save as high precision average
  clrf	XHI			; reset 16 bit X sum
  clrf	XLO
  goto	int_done


; ------ (30)

ych_2			
  movf	NSUM,W			; see if enough samples in Y sum
  btfss	STATUS,Z
  goto	int_done
norm_y
  rlf	YLO,F			; shift 16 bit value up by 2
  rlf	YHI,F
  rlf	YLO,F
  rlf	YHI,F
  movf	YHI,W			; save as low precision average
  movwf	YAVG
  clrf	YAV4			; default x4 value is zero
  movlw	96			; check if Y value too low
  subwf	YHI,F
  btfss STATUS,C
  goto	ny_done
scale_y
  movlw	0xFF			; default x4 value is now 255
  rlf	YLO,F			; scale up resolution by 4
  rlf	YHI,F
  btfsc	STATUS,C		; see if too high
  goto	ny_done
  rlf	YLO,F
  rlf	YHI,F
  btfss	STATUS,C		; see if too high
  movf	YHI,W
ny_done
  movwf	YAV4			; save as high precision average
  clrf	YHI			; reset 16 bit Y sum
  clrf	YLO
  movlw	64			; set up to collect 64 new samples
  movwf	NSUM
  goto	int_done


; ------ PHASES 1 & 3 ------ (5)

harvest
  bcf	FLAGS,AGET		; start new conversion on next cycle
  movf	ADRESH,W		; get analog value
  btfsc ADCON0,CHS1		; see if AN0 or AN2 was converted		
  goto 	ych_3


; ------- (21)

xch_1	
  bsf	ADCON0,CHS1		; set up to convert Y next (AN2)
  movwf	XVAL			; save X reading
  movwf	DIFF			; get absolute difference from X average
  subwf	XAVG,W			
  btfsc	STATUS,C
  goto	xdiff
  movf	XAVG,W
  subwf	DIFF,W
xdiff
  movwf	DIFF
  sublw	ATOL			; see if far from average
  btfss	STATUS,C
  bsf	FLAGS,SKIP		; remember to not update sums
  movf	DIFF,W			; see if bigger than current peak
  subwf	PEAK,W
  btfsc	STATUS,C		
  goto	int_done
  movf	DIFF,W			; save new peak value
  movwf	PEAK
  bsf	FLAGS,XFER		; set up to record values at peak
  goto	int_done


; ------- (21 + 7 + 14)

ych_3
  bcf	ADCON0,CHS1		; set up to convert X next (AN0)
  movwf	YVAL			; save Y reading
  movwf	DIFF			; get absolute difference from Y average
  subwf	YAVG,W			
  btfsc	STATUS,C
  goto	ydiff
  movf	YAVG,W
  subwf	DIFF,W
ydiff
  movwf	DIFF
  sublw	ATOL			; see if far from average
  btfss	STATUS,C
  bsf	FLAGS,SKIP		; remember to not update sums
  movf	DIFF,W			; see if bigger than current peak
  subwf	PEAK,W
  btfsc	STATUS,C
  goto	shock
  movf	DIFF,W			; save new peak value
  movwf	PEAK
  bsf	FLAGS,XFER		; set up to record values at peak

shock
  btfss	FLAGS,XFER		; see if new peak occurred (X or Y)
  goto 	sums
  movf	XVAL,W			; save current X value
  movwf	XPK
  movf	YVAL,W			; save current Y value
  movwf	YPK
  bcf	FLAGS,XFER		; mark transfer as done

sums
  btfss	FLAGS,SKIP		; see if values should be added in
  goto	update
  bcf	FLAGS,SKIP		; hope next sample will be better
  decfsz MISS,F			; see if too many missed
  goto	int_done
  movlw	0x80			; reinitialize averages
  movwf	XAVG
  movwf	YAVG
  movlw	GAP			; reset bad sample counter
  movwf	MISS
  goto	int_done
update
  movf	XVAL,W			; increment 16 bit X sum
  addwf	XLO,F			
  btfsc	STATUS,C
  incf	XHI,F
  movf	YVAL,W			; increment 16 bit Y sum
  addwf	YLO,F			
  btfsc	STATUS,C
  incf	YHI,F
  decf	NSUM,F			; remaining samples to collect
  movlw	GAP			; these samples were good
  movwf	MISS


; ------ RESTORE ------ (7)

int_done
  swapf	S_SAVE,W		; restore STATUS and W registers
  movwf	STATUS
  swapf	W_SAVE,F
  swapf	W_SAVE,W
  bcf	PORTC,LED		; signal interrupt done
  retfie


; ==================================================================

; main loop handles serial communication to/from servos and host

start
  call	sys_init		; set up serial, port bits, etc.

main
  call  cmd_rd			; wait for request
prompt
  movf	ID0,W			; see if all servos read yet
  subwf	IDN,W
  btfss	STATUS,C
  goto  main
  call	request			; ask for servo N's data
  movlw	DUMP			; wait for return packet (sync + ID + data)
  call	drain
  incf	ID0,F			; advance to next number
  goto  prompt
	

; request position, velocity, and force from some servo
; servo to read is passed in ID0 register

request
  call tx_start			; >> start AX-12 format packet
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
  call tx_end			; >> finish AX-12 format packet
  return


; ------------------------------------------------------------------

; wait for a fully formed multi-read request (saves ID0 and IDN)
; looks for correctly formed packet and checksum, resyncs if needed
; only returns if proper 0x84 request (0x85 handled internally)

cmd_rd
  btfsc	RCSTA,OERR		; clear any overrun error
  bcf	RCSTA,CREN
  bsf	RCSTA,CREN		; make sure receiver is enabled
sync
  call  rx			; read first sync byte
  btfsc	STATUS,Z
  goto	sync
  sublw 0xFF
  btfss	STATUS,Z
  goto	sync
  call 	rx			; read second sync byte
  btfsc	STATUS,Z
  goto	sync
  sublw 0xFF
  btfss	STATUS,Z
  goto	sync
chk_id
  clrf	CHK			; clear checksum
  call 	rx			; see if packet for this PIC
  btfsc	STATUS,Z
  goto	sync
  sublw	ME
  btfsc	STATUS,Z
  goto 	chk_len
  call	rx			; get size of rest of bad packet
  btfss	STATUS,Z
  call	drain			
  goto	sync
chk_len
  call	rx			; save packet length and check
  btfsc	STATUS,Z
  goto	sync
  movwf	CNT
  sublw 2                       ; 2 bytes for ID request
  btfsc STATUS,Z
  goto  who_cmd
  movf  CNT,W
  sublw 4                       ; 4 bytes for mega-read (+ accel)
  btfsc	STATUS,Z
  goto	mega_cmd
  movf	CNT,W			; get rid of rest of bad packet
  call	drain
  goto	sync

who_cmd
  call  rx                      ; see if proper ID request command
  btfsc	STATUS,Z
  goto	sync
  sublw WHO			; command 0x86
  btfsc STATUS,Z
  goto  chk_who
  decf	CNT,W			; get rid of rest of bad packet
  call 	drain
  goto	sync
chk_who
  comf	CHK,W			; save checksum so far (inverted)
  movwf	TST
  call	rx			; get packet checksum
  btfsc	STATUS,Z
  goto	sync
  subwf	TST,W			; see if checksum matches
  btfsc	STATUS,Z
  call	send_who
  goto	sync			; no return since not mega-read

mega_cmd
  call	rx			; see if proper mega-read command 
  btfsc	STATUS,Z
  goto	sync
  movwf	CMD			; save command
  andlw	0xFE
  sublw	PVF			; command 0x84 or 0x85 (ignore bit 0)
  btfsc	STATUS,Z
  goto	get_args
  decf	CNT,W			; get rid of rest of bad packet
  call 	drain
  goto	sync
get_args
  call	rx			; save starting servo number
  btfsc	STATUS,Z
  goto	sync
  movwf	ID0
  call	rx			; save ending servo number
  btfsc	STATUS,Z
  goto	sync
  movwf	IDN
chk_mega
  comf	CHK,W			; save checksum so far (inverted)
  movwf	TST
  call	rx			; get packet checksum
  btfsc	STATUS,Z
  goto	sync
  subwf	TST,W			; see if checksum matches
  btfss	STATUS,Z
  goto	sync
  btfsc	CMD,0			; check for command 0x85 vs 0x84
  call	send_acc
  return


; report accelerometer data in appropriate packet format (command 0x85)

send_acc
  bcf	INTCON,GIE		; temporarily disable interrupts
  clrf	PEAK			; clear peak threshold 
  bcf	FLAGS,XFER		; cancel any pending recording
  movf	XPK,W			; copy current peaks
  movwf	XPK2
  movf	YPK,W
  movwf YPK2
  movlw	ACMAX			; restart peak interval timeout
  movwf	ACTIM
  bsf	INTCON,GIE		; re-enable interrupts
acc_data
  call	tx_start		; >> start AX-12 format packet
  movlw	ME			; send PIC controller ID
  call  tx
  movlw 5			; response length (always 5)
  call  tx
  movf	XPK2,W			; send readings from last peak
  call	tx
  movf	YPK2,W
  call	tx
  movf	XAV4,W			; send current averages (x4)
  call	tx
  movf	YAV4,W
  call	tx
  call 	tx_end			; >> finish AX-12 format packet
  return


; report body ID in appropriate packet format (command 0x86)

send_who
  call  tx_start		; >> start AX-12 format packet
  movlw	ME			; send PIC controller ID
  call  tx
  movlw 2			; response length (always 2)
  call  tx
  movlw ROBOT                   ; get hardcoded body ID
  call  tx
  call  tx_end			; >> finish AX-12 format packet
  return


; ------------------------------------------------------------------

; carefully read a byte from receiver to W and add to checksum
; timeout is signaled by Z flag being set on return

rx
  movlw	RXMAX			; set timeout to max
  movwf	RXTIM
rx_wait
  movf	RXTIM,F			; check if timeout occurred
  btfsc	STATUS,Z
  return                        ; return with Z flag set
  btfss PIR1,RCIF		; see if a byte came in
  goto 	rx_wait
  btfss	RCSTA,FERR		; check for framing error
  goto	rx_ok
  movf	RCREG,W			; remove bad byte and try again
  goto	rx
rx_ok
  movf	RCREG,W			; get byte into accumulator
  addwf	CHK,F			; add into checksum
  bcf	STATUS,Z		; clear Z flag for success
  return


; wait for number of bytes in W to be received (no error checking)
; potentially times out and PIC resets if bad bytes or too few received

drain
  movwf	CNT			; initialize count
rx_all
  movlw	RXMAX                   ; set up timeout
  movwf	RXTIM
rx_next
  movf	RXTIM,F			; check for timeout
  btfsc	STATUS,Z
  return
  btfss PIR1,RCIF		; see if a byte came in
  goto 	rx_next
  movf	RCREG,W			; clear receive flag and ignore byte
  decfsz CNT,F			; see if done yet
  goto 	rx_all
  return


; ------------------------------------------------------------------

; disable receiver then transmit packet header

tx_start
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
  return


; send byte in W to transmitter 
; also adds to ongoing checksum AFTER transmission

tx
  btfss	PIR1,TXIF		; wait for buffer to be ready
  goto 	tx
  movwf	TXREG			; queue for transmission
  addwf	CHK,F			; update checksum
  return


; transmit packet checksum then enable receiver

tx_end
  comf	CHK,W			; send checksum (inverted)
  call	tx
tx_done
  btfss	TXSTA,TRMT		; wait for last byte to finish
  goto	tx_done
  bcf	PORTC,TXPIN		; turn transmitter off
  bsf	RCSTA,CREN		; start listening again
  return


; ------------------------------------------------------------------
; initial setup of ports, timers, A/D converter, etc.
; timer0 using Fosc/8 = 128us interrupt, x256 = 32.8ms timeout

sys_init
  clrf	PORTC			; make sure all outputs are low	
  movlw 0x24			; serial at Fosc/16 = 1Mbaud
  movwf	TXSTA
  movlw 0x90			; enable serial receiver
  movwf	RCSTA
  movlw	0x41			; top 8 bits, ext Vref, A/D on
  movwf	ADCON0
sys_bank
  bsf   STATUS,RP0		; -- switch to register bank 1
  movlw 0xD0		 	; timer0 at Fosc/8 with prescaler
  movwf	OPTION_REG
  movlw	0x3F			; set tri-state enable, LED as digital
  movwf	ANSEL
  movlw	0xF3			; set tri-state enable, LED as outputs
  movwf	TRISC
  movlw	0x20			; convert at Tad = fosc/32 = 2us
  movwf	ADCON1
  bcf 	STATUS,RP0		; -- switch back to register bank 0

sys_vars
  clrf	XHI			; clear 16 bit X sum
  clrf	XLO
  clrf	YHI			; clear 16 bit Y sum
  clrf	YLO
  movlw	64			; set up to collect 64 samples
  movwf	NSUM			
  movlw	0x80			; init accelerometer averages
  movwf	XAVG
  movwf	YAVG
  movwf	XAV4                    ; high resolution also
  movwf	YAV4
  movwf	XPK			; init accelerometer peak values
  movwf YPK
  clrf	PEAK			; no shocks yet
  clrf	FLAGS			; ready to start A/D conversion 

sys_time
  movlw	GAP			; tolerate some skipped averages
  movwf	MISS			
  clrf	ACTIM			; peak values fresh
  clrf	RXTIM			; not waiting for receive
  clrf	TMR0			; no overflow yet
  movlw	0xA0			; interrupt on timer0 (128us)
  movwf	INTCON
  return


; ==================================================================

  END

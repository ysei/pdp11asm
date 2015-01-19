; Miner for BK0010 / ����� ��� ��0010
; (c) 5-03-2012 VINXRU (aleksey.f.morozov@gmail.com)

		CONVERT1251TOKOI8R
	        ORG 01000

;----------------------------------------------------------------------------        
; ����        

EntryPoint:      ; ���� ����� ���������� �� �����
		MOV #16384, SP

		; ��������� ������ 256x256
		MOV #0233, R0
		EMT 016

		; ���������� �������
		MOV #0232, R0
		EMT 016

		; ������ �������
		MOV #731, @#0177706
		MOV #0160, @#0177712

		; ��������� ���������� ����������
		MOV #64, @#0177660

		; ������� ������
Menu:		JSR PC, @#clearScreen

		; ����� ����		
		MOV #bmpLogo, R0
		MOV #045020, R1
		MOV #37, R3
drawLogo0:	MOV #16, R2
drawLogo1:	MOV (R0)+,(R1)+
		SOB R2, drawLogo1
		ADD #32, R1
		SOB R3, drawLogo0

		; ����� ����
		MOV #txtMenu, R0
		JSR PC, @#Print

MenuLoop:	; �������������� ��������� ��������� �����.
                JSR PC, @#rand

		; ������������ �������
		MOV @#0177662, R0

		; ������ �������
		MOV #menuItems, R5
Menu2:		MOV (R5)+, R1
		BEQ MenuLoop
		CMPB R0, R1
		BEQ StartGame
		ADD #8, R5
		BR Menu2

;----------------------------------------------------------------------------
; ������ ����

txtMenu:	DB 10,12,0222,"0. ������",0
		DB 10,13,0222,"1. �������",0
		DB 10,14,     "2. ��������",0
		DB 10,15,     "3. ������������",0
		DB  9,22,0221,"(c) 2012 VINXRU",0
		DB  3,23,0223,"aleksey.f.morozov@gmail.com",0,255

menuItems:	DW '0', 9, 9, 3, 21006	; �������, ������, ������, ���-�� ����, ��������� �� ������
		DW '1', 9, 9, 10, 21006
	 	DW '2', 13, 10, 20, 20486
		DW '3', 16, 14, 43, 18432
		DW 0

;----------------------------------------------------------------------------
; ����

StartGame:      ; ��������� �������� ����
		MOV (R5)+, gameWidth
		MOV (R5)+, gameHeight
		MOV (R5)+, bombsCnt
		MOV (R5)+, playfieldVA

		; ������� ������
		JSR PC, @#clearScreen
		JSR PC, @#fillBlocks

		; ������������� ������ � ����� ����
		MOV gameWidth, R0
		ASR R0
		MOV R0, cursorX
		MOV gameHeight, R0
		ASR R0
		MOV R0, cursorY

		; ������� �������� ����������
		CLR bombsPutted
		CLR gameOverFlag
		CLR time

		; ������� �������� ����
		CLR R0
		MOV #254, R1
StartGame1:	MOVB R1, playfield(R0)
		CLRB userMarks(R0)
		INCB R0
		BNE StartGame1

		; ����� ��������
		MOV #bmpGood, R0
		JSR PC, @#drawSmile

		; ��������� �������� ����
		JSR PC, @#drawPlayField
	
		; ����� �����
		JSR PC, @#leftNumber
		JSR PC, @#rightNumber
		
		; �����
mainLoop:       CMP gameOverFlag, #1
		BEQ mainLoop1
		CMP time, #999
		BEQ mainLoop1

		; ������ �������?
		CLR R0
		CMP @#0177710, #365
		ADC R0
		CMP lastTimer, R0
		BEQ mainLoop1
		MOV R0, lastTimer

		; �� ������. ����������� ���������� � �������������� �����
		INC time
		JSR PC, @#rightNumber

mainLoop1:      ; ������ �� �������
		BIT #128, @#0177660
		BEQ mainLoop

		; ����������� ����������
		MOV @#0177662, R0
		CMP R0, #8
		BEQ cursorLeft
		CMP R0, #0x19
		BEQ cursorRight
		CMP R0, #0x1A
		BEQ cursorUp
		CMP R0, #0x1B
		BEQ cursorDown	
		CMP R0, #' '
		BEQ leftClick

		BR rightClick

;----------------------------------------------------------------------------
; ����� ������ ����

leftClick:	; ���� ����� �� �����������, ���������� �����
		MOV bombsPutted, R0
		BEQ putBombs

		; ���� ���� ���������, �� ����� � ����
putBombsRet:	CMP gameOverFlag, #1
		BEQ MenuFar

		; ��������� ������� Open
		MOV cursorX, R0
		MOV cursorY, R1
		JSR PC, @#open

		; �������� ������ �����
		JSR PC, @#drawCursor

		; ���������, �������� �� ��
		JSR PC, @#checkWin
	
		; �������� ���� ����	
		BR mainLoop

;----------------------------------------------------------------------------

MenuFar:	JMP @#Menu

;----------------------------------------------------------------------------

cursorLeft:     CMP cursorX, #0
		BEQ mainLoop
		JSR PC, @#hideCursor
		DEC cursorX
		JSR PC, @#drawCursor
		BR mainLoop

;----------------------------------------------------------------------------

cursorRight:    MOV cursorX, R0
		INC R0
		CMP R0, gameWidth
		BEQ mainLoop
		JSR PC, @#hideCursor
		INC cursorX
		JSR PC, @#drawCursor
		BR mainLoop

;----------------------------------------------------------------------------

cursorUp:    	CMP cursorY, #0
		BEQ mainLoop
		JSR PC, @#hideCursor
		DEC cursorY
		JSR PC, @#drawCursor
		BR mainLoop

;----------------------------------------------------------------------------

cursorDown:	MOV cursorY, R0
		INC R0
		CMP R0, gameHeight
		BEQ mainLoop
		JSR PC, @#hideCursor
		INC cursorY
		JSR PC, @#drawCursor
		BR mainLoop

;----------------------------------------------------------------------------
; ����� ������ ���������� - ��������� �����

rightClick:	; if(gameOver) return;
		CMP gameOverFlag, #1
		BEQ mainLoop
		; a=x+y*miner_w
		MOV cursorX, R0
		MOV cursorY, R1
		JSR PC, @#mul01
		; userMarks[a] = (userMarks[a]+1)%3;
		MOVB userMarks(R2), R3
		INC R3
		CMP R3, #4
		BNE rightClick1
		  CLR R3 
rightClick1:	MOVB R3, userMarks(R2)
		; redraw();
		JSR PC, @#hideCursor
		JSR PC, @#drawCursor
rightClickRet:	JSR PC, @#leftNumber
	 	JMP @#mainLoop

;----------------------------------------------------------------------------
; ��������� ����� �� ����

putBombs:	INC bombsPutted

                ; ����
		MOV bombsCnt, R5
putBombs1:      MOV R5, -(SP)
	
                ; ���������� X
putBombs2:	JSR PC, @#rand
		MOV gameHeight, R1
		JSR PC, @#div
		MOV R0, R5

                ; ���������� Y
		JSR PC, @#rand
		MOV gameWidth, R1
		JSR PC, @#div

		; ����� �� ������ ���� ��� �������
		CMP cursorY, R5
		BNE putBombs3
		CMP cursorX, R0
		BEQ putBombs2

		; ������ ������ � �������
putBombs3:	MOV R5, R1
		JSR PC, @#mul01

		; ����� � ���� ������ ��� ����
		CMPB #255, playfield(R2)
		BEQ putBombs2

		; ������ �����
		MOVB #255, playfield(R2)

		MOV (SP)+, R5
		SOB R5, putBombs1

		JMP @#putBombsRet

;----------------------------------------------------------------------------
; ������������ ������ (������� ������)

hideCursor:	MOV cursorX, R0
		MOV cursorY, R1
		JSR PC, @#mul01		; R0+R1*16 -> R2
		JSR PC, @#calcCell2	; R0,R1 -> R1. ������ R3
		MOVB userMarks(R2), R5
		MOVB playfield(R2), R2
		JSR PC, @#getBitmap
		CMP R0, #bmpUn
		BEQ drawCursor4
drawCursor5:	JSR PC, @#drawImage ; ������������ R0,R1. ������ R2.
                RTS PC

;................................

drawCursor4:	CMP R5, #1
		BEQ drawCursorF
		CMP R5, #2
		BNE drawCursor5
		mov #bmpQ, R0
		BR drawCursor5

;................................
	
drawCursorF:	mov #bmpF, R0
		BR drawCursor5

;----------------------------------------------------------------------------
; ���������� ������ ������ ������

drawCursor:	MOV cursorX, R0
		MOV cursorY, R1
		JSR PC, @#calcCell2
		MOV #bmpCursor, R0
		JSR PC, @#drawTransImage
		RTS PC      

;----------------------------------------------------------------------------
; ������ ������ � ����������� ������ �������� ���� 
; R0,R1 - ���������� => R1 - �����, ������ R3

calcCell2:	MOV R1, R3
		SWAB R3
		ROL R3
		ROL R3
		ADD R0, R3
		ADD R0, R3
		ADD R0, R3
		ADD R0, R3
		ADD playfieldVA, R3		
		MOV R3, R1		
		RTS PC

;----------------------------------------------------------------------------
; ������ ����
; R0,R1 - ����������. R3 - ������� => R2 - ������

check:		; if(x>=8 || y>=8) return;
		CMP R0, gameWidth
		BCC checkRet
                CMP R1, gameHeight
		BCC checkRet
		; a = x+y*miner_w
		JSR PC, @#Mul01
		; if(playfield[a]==-1) R3++;
		CMPB playfield(R2), #255 
		BNE openRet
		INC R3
checkRet:	RTS PC

;----------------------------------------------------------------------------
; ������ ����

call8:		DEC R1
		JSR PC, @#call81
		DEC R0
		INC R1
		JSR PC, (R5)
		INC R0
		INC R0
		JSR PC, (R5)
		DEC R0
		INC R1
		JSR PC, @#call81
		DEC R1
		RTS PC

;----------------------------------------------------------------------------
; ������ ����

call81:         DEC R0
		JSR PC, (R5)
		INC R0
		JSR PC, (R5)
		INC R0
		JSR PC, (R5)
		DEC R0
		RTS PC

;----------------------------------------------------------------------------
; R0+R1*16 => R2

mul01:		MOV R1, R2
		ASL R2
		ASL R2
		ASL R2
		ASL R2
		ADD R0, R2
		RTS PC

;----------------------------------------------------------------------------
; ������ ����

open:		CMP R0, gameWidth
		BCC openRet
                CMP R1, gameHeight
		BCC openRet
		; a=x+y*miner_w 
		JSR PC, @#Mul01
	        ; if(playfield[a]==-1) miner_gameOver=true;
	        CMPB playfield(R2), #255
	        BEQ die
	        ; if(playfield[a]!=-2) return;
		CMPB playfield(R2), #254
		BNE openRet
		; push
		MOV R5, -(SP)
                MOV R0, -(SP)
                MOV R1, -(SP)             
		; playfield[a]=call8(check,x,y);
		MOV #check, R5
		CLR R3
		JSR PC, @#call8
		JSR PC, @#mul01
		MOVB R3, playfield(R2)
		; redraw
		MOV R3, R4
		JSR PC, @#redrawCell012
		; if(playfield[a]!=0) return;
		MOV R4, R4
		BNE openRets
		; call8(open,x,y);
                MOV (SP)+, R1
                MOV (SP)+, R0
		MOV #open, R5
		JSR PC, @#call8
                MOV (SP)+, R5
		RTS PC

openRets:       MOV (SP)+, R1
                MOV (SP)+, R0
		MOV (SP)+, R5
openRet:        RTS PC

;----------------------------------------------------------------------------
; ����� ����

die:		; ����� ��������
		MOV #bmpBad, R0
		JSR PC, @#drawSmile
		JMP @#gameOver

;----------------------------------------------------------------------------
; ������������ ������ �� ������
; R0,R1 - ����������. R2 - �����. => ������ ��� ��������

redrawCell012:   MOV R3, R2
		 JSR PC, @#calcCell2 ; R0,R1 -> R1. ������ R3
		 JSR PC, @#getBitmap ; R2 -> R0		
		 JSR PC, @#drawImage ; ������������ R0,R1. ������ R2.
		 RTS PC

;----------------------------------------------------------------------------
; �������� ��������� �� ����������� �� ������ �����������
; R0 => R0

getBitmap:      MOV R2, R0
		INC R0
		INC R0
		CMP gameOverFlag, #1
		BNE getBitmap3		
getBitmap2:	SWAB R0
		ASR R0
		ASR R0
		ADD #bmpUn, R0
                RTS PC

getBitmap3:     CMPB R0, #1
		BNE getBitmap2
		DEC R0
		BR getBitmap2

;----------------------------------------------------------------------------
; ��������� ��������� �����
; ��� => R0 - ��������� �����. R1 - ������.

rand_state:	dw 0x1245
		
rand:           MOV rand_state, R0
		MOV R0, R1
		ASL R0
		ASL R0
		ASR R1                    
		ASR R1                    
		ASR R1                    
		ASR R1
		ASR R1
		XOR R1, R0
		MOV R0, rand_state

		MOV R0, R1
		SWAB R0
		XOR R1, R0
		MOV @#0177710, R1
		XOR R1, R0
		BIC #0xFF00, R0

		RTS PC

;----------------------------------------------------------------------------
; �������
; R0/R1 => R2, ������� � R0

div:		CLR R2
div1:		SUB R1, R0
		BCS div2
		INC R2
		BR div1
div2:		ADD R1, R0
		RTS PC

;----------------------------------------------------------------------------
; ��������, ������� �� �����
; => ������ ��� ��������

checkWin:	; ������� �� �������� ������ ��� ����.
		MOV #254, R3
		CLR R1
checkWin2:	 CLR R0
checkWin1:	  JSR PC, @#mul01
		  CMPB playfield(R2), R3
		  BEQ checkWin3
	  	 INC R0
  	  	 CMP gameWidth, R0
	  	 BNE checkWin1
		INC R1
		CMP gameHeight, R1
		BNE checkWin2
		
		; ������������ �������

		; ������ �������
		MOV #bmpWin, R0
		JSR PC, @#drawSmile

gameOver:	; ����� ����
		MOV #1, R0
		MOV R0, gameOverFlag

		; �������� ��� �����
		JSR PC, @#drawPlayField

checkWin3:	RTS PC
                               
;----------------------------------------------------------------------------
; ������ � ����� �� ����� ������ �����
; => ������ ��� ��������

leftNumber:	; ������� ���-�� ������
		CLR R0
		MOV bombsCnt, R1
leftNumber1:	 CMPB userMarks(R0), #1
		 BNE leftNumber3
		  DEC R1
		  BEQ leftNumber4
leftNumber3:	INCB R0
		BNE leftNumber1

		; ����� ����� �� �����
leftNumber4:	MOV R1, R0
	        MOV #040510, R3		; ����� � �����������
		JSR PC, @#drawNumber

		RTS PC

;----------------------------------------------------------------------------
; ����� �� ����� ������� �����
; => ������ ��� ��������
		          
rightNumber:	MOV time, R0		; �������� �����
	        MOV #040573, R3		; ����� � �����������
		JSR PC, @#drawNumber
		RTS PC

;----------------------------------------------------------------------------
; ����� ������������ ����� �� �����
; R0 - �����, R3 - ����� � �����������. => ������ ��� ��������.

drawNumber:	MOV #3, R5		; ���-�� �����

drawNumber0:	; �������� ������ �����
		MOV #10, R1		
		JSR PC, @#div

		; ������ ��������� �������
		SWAB R0
		ASR R0
		ASR R0
		ADD #bmpN0, R0

		; ����� ����������
		MOV #21, R4
drawNumber1:    MOVB (R0)+, (R3)+
		MOVB (R0)+, (R3)+
		MOVB (R0)+, (R3)+
		ADD #61, R3           
		SOB R4, drawNumber1

		; ��������� ���������� ��������
		SUB #1347, R3

		; ����
		MOV R2, R0
		SOB R5, drawNumber0
		
		RTS PC

;-----------------------------------------------
; ����� ��������
; => R0 - �����������. ������ R1,R2

drawSmile:	MOV #040435, R1
		MOV #24, R2
drawGood:	MOV (R0)+, (R1)+
		MOV (R0)+, (R1)+
		MOV (R0)+, (R1)+
		ADD #58, R1
		SOB R2, drawGood
		RTS PC

;----------------------------------------------------------------------------
; ��������� �������� ���� � �������
; => ������ ��� ��������

drawPlayField:	CLR R1
LOOP2:    	 CLR R0
LOOP1:	    	  MOV R0, -(SP)
	    	  MOV R1, -(SP)
  	     	   JSR PC, @#mul01
		   MOVB playfield(R2), R3
  	     	   JSR PC, @#redrawCell012
	    	  MOV (SP)+, R1
	    	  MOV (SP)+, R0
	  	 INC R0
  	  	 CMP gameWidth, R0
	  	 BNE LOOP1
		INC R1
		CMP gameHeight, R1
		BNE LOOP2
		
		JMP @#drawCursor

;----------------------------------------------------------------------------
; �������� �����
; => ������ R0, R2

clearScreen:	MOV #040000, R0
		MOV #2048, R2
clearScreen1:	CLR (R0)+
		CLR (R0)+
		CLR (R0)+
		CLR (R0)+
		SOB R2, clearScreen1
		RTS PC

;----------------------------------------------------------------------------
; ��������� �����

fillBlocks:	MOV #044000, R0
		MOV #14, R4
fillBlocks3:	MOV #bmpBlock, R1
		MOV #16, R3
fillBlocks2:	MOV #16, R2
fillBlocks1:	MOV (R1)+, (R0)+
		MOV (R1)+, (R0)+
		SUB #4, R1
		SOB R2, fillBlocks1
		ADD #4, R1
		SOB R3, fillBlocks2
		SOB R4, fillBlocks3
		RTS PC

;----------------------------------------------------------------------------
; ���������� ����������� 16x16 � �������������
; R0 - �����������, R1 - ���� => ������ R1, R2

drawTransImage: MOV     #16, R2
drawTransImag1:	BIC     (R0)+, (R1)
		BIS     (R0)+, (R1)+
		BIC     (R0)+, (R1)
		BIS     (R0)+, (R1)+
        	ADD     #60, R1                        
		SOB	R2, drawTransImag1
		RTS	PC

;----------------------------------------------------------------------------
; ���������� ����������� 16x16
; R0 - �����������, R1 - ���� => ������ R1, R2

drawImage:      MOV     #16, R2
drawImage1:	MOV     (R0)+, (R1)+
		MOV     (R0)+, (R1)+
        	ADD     #60, R1
		SOB	R2, drawImage1
		RTS	PC

;----------------------------------------------------------------------------
; ����� ������

Print:		; ��������� ���������
		CLR R1
		MOVB (R0)+, R1
		CLR R2
		MOVB (R0)+, R2
		EMT 024		                       

		; ����� ������
		MOV R0, R1
		MOV #0x00FF, R2
		EMT 020

		; ����� ���� ������
Print1:		MOVB (R0)+, R1
		BNE Print1
		CMPB (R0), #255
		BNE Print

		RTS PC

;----------------------------------------------------------------------------
; �����������

bmpLogo:   	insert_bitmap2 "resources/logo.bmp",  128, 37

bmpCursor:  	insert_bitmap2t "resources/cursor.bmp",  16, 16

bmpF:    	insert_bitmap2 "resources/f.bmp", 16, 16
bmpQ:    	insert_bitmap2 "resources/q.bmp", 16, 16

bmpUn:   	insert_bitmap2 "resources/un.bmp", 16, 16
bmpB:    	insert_bitmap2 "resources/b.bmp",  16, 16
bmp0:    	insert_bitmap2 "resources/0.bmp",  16, 16
bmp1:    	insert_bitmap2 "resources/1.bmp",  16, 16
bmp2:    	insert_bitmap2 "resources/2.bmp",  16, 16
bmp3:    	insert_bitmap2 "resources/3.bmp",  16, 16
bmp4:    	insert_bitmap2 "resources/4.bmp",  16, 16
bmp5:    	insert_bitmap2 "resources/5.bmp",  16, 16
bmp6:    	insert_bitmap2 "resources/6.bmp",  16, 16
bmp7:    	insert_bitmap2 "resources/7.bmp",  16, 16
bmp8:    	insert_bitmap2 "resources/8.bmp",  16, 16

bmpGood: 	insert_bitmap2 "resources/good.bmp", 24, 24
bmpBad:  	insert_bitmap2 "resources/bad.bmp", 24, 24
bmpWin:  	insert_bitmap2 "resources/win.bmp", 24, 24

bmpN0:   	insert_bitmap2 "resources/n0.bmp", 12, 21
bmpN1:   	insert_bitmap2 "resources/n1.bmp", 12, 21
bmpN2:   	insert_bitmap2 "resources/n2.bmp", 12, 21
bmpN3:   	insert_bitmap2 "resources/n3.bmp", 12, 21
bmpN4:   	insert_bitmap2 "resources/n4.bmp", 12, 21
bmpN5:   	insert_bitmap2 "resources/n5.bmp", 12, 21
bmpN6:   	insert_bitmap2 "resources/n6.bmp", 12, 21
bmpN7:   	insert_bitmap2 "resources/n7.bmp", 12, 21
bmpN8:   	insert_bitmap2 "resources/n8.bmp", 12, 21
bmpN9:   	insert_bitmap2 "resources/n9.bmp", 12, 21                          

bmpBlock:   	insert_bitmap2 "resources/block.bmp", 16, 16                       

endOfROM:              

;-----------------------------------------------

pfSize equ 256 ; ������� ����� ��� ������, ���� ������������ ������ ���� 16x14

gameWidth:	dw 0
gameHeight:	dw 0
gameOverFlag:  	dw 0
cursorX:    	dw 0
cursorY:    	dw 0
playfieldVA:	dw 0
bombsCnt:   	dw 0
bombsPutted:	dw 0
time:	    	dw 0
lastTimer:      dw 0
playfield:    	db pfSize dup(0)
userMarks:    	db pfSize dup(0)

;-----------------------------------------------

make_bk0010_rom "bk0010_miner.bin", EntryPoint, endOfROM
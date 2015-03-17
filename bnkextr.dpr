Program bnktest;
{$APPTYPE CONSOLE}
(*
http://www.geocities.jp/aoyoume/aotuv/index.html
http://rarewares.org/ogg-oggenc.php#oggenc-aotuv
http://www.eveonline.com/ingameboard.asp?a=topic&threadID=1018956
http://forum.xentax.com/viewtopic.php?f=17&t=3477

.BNK Format specifications

char {4} - header (BKHD) // BanK HeaDer
uint32 {4} - size of BKHD
uint32 {4} - unknow (version?)
uint32 {4} - unknow
uint32 {4} - unknow
uint32 {4} - unknow
byte {x} - zero padding (if any)

char {4} - header (DIDX) // Data InDeX
uint32 {4} - size of DIDX
following by records 12 bytes each:
  uint32 {4} - unknow
  uint32 {4} - relative file offset from start of DATA, 16 bytes aligned
  uint32 {4} - file size

char {4} - header (DATA)
uint32 {4} - size of DATA

char {4} - header (HIRC) // ???
uint32 {4} - size of HIRC

char {4} - header (STID) // Sound Type ID
uint32 {4} - size of STID
uint32 {4} - Always 1?
uint32 {4} - Always 1?
uint32 {4} - unknow
byte {1} - TID Length (TL)
char {TL} - TID string (usually same as filename, but without extension)

Init.bnk
STMG
HIRC
FXPR
ENVS
*)
Type
     TSect = Packed Record
               Sign: Array[0..3] Of Char;
               Size: Integer;
             End;

     TIDX = Packed Record
              Unkn: Integer;
              Offs: Integer;
              Size: Integer;
            End;

Var
    I, DT: Integer;
    S, SI: String;
    Fl, F: File;
       FR: Array Of TIDX;
       CS: TSect;
        P: Pointer;

function swap32(const dw: longint): longint; assembler;
asm
  bswap eax
end;

Function Int03Str(N: Integer): String;
Begin
  Str(N, result);
  While Length(result) < 3 Do result:='0' + result;
End;

Begin
  WriteLn('Divinity 2: Ego Draconis / Army of Two .BNK extractor');
  WriteLn('(c) CTPAX-X Team 2009-2010');
  WriteLn('http://www.CTPAX-X.org');
  WriteLn;
  If ((ParamCount < 1)  Or (ParamCount > 2)) Then
    Begin
      WriteLn('Usage: bnkextr filename.bnk [/swap]');
      WriteLn('/swap - swap byte order (use it for unpacking AoT)');
      Exit;
    End;
  AssignFile(Fl, ParamStr(1));
  FileMode:=0;
  {$I-}
  Reset(Fl, 1);
  {$I+}
  FileMode:=2;
  If IOResult <> 0 Then
    Begin
      WriteLn('Can''t open input file: ' + ParamStr(1));
      Exit;
    End;
  // parse file structure
  SetLength(FR, 0);
  DT:=0;
  SI:='';
  While Not EOF(Fl) Do
    Begin
      BlockRead(Fl, CS, SizeOf(TSect));
      If ParamCount > 1 Then CS.Size:=swap32(CS.Size);
//      WriteLn(CS.Sign, ': ', CS.Size);
      If CS.Sign = 'DIDX' Then
        Begin
          SetLength(FR, CS.Size Div SizeOf(FR[0]));
          BlockRead(Fl, FR[0], CS.Size);
          Continue;
        End;
      If CS.Sign = 'STID' Then
        Begin
          Seek(Fl, FilePos(Fl) + 12);
          I:=0;
          BlockRead(Fl, I, 1);
          SetLength(SI, I);
          BlockRead(Fl, SI[1], I);
          Continue;
        End;
      If CS.Sign = 'DATA' Then DT:=FilePos(Fl);
      Seek(Fl, FilePos(Fl) + CS.Size);
    End;
  // extract files
  If ((DT > 0) And (Length(FR) > 0)) Then
    For I:=0 To Length(FR)-1 Do
      Begin
        S:=SI + '.' + Int03Str(I + 1) + '.wav';
        Write(S);
        If ParamCount > 1 Then
          Begin
            FR[I].Size:=swap32(FR[I].Size);
            FR[I].Offs:=swap32(FR[I].Offs);
          End;
        Seek(Fl, DT + FR[I].Offs);
        GetMem(P, FR[I].Size);
        BlockRead(Fl, P^, FR[I].Size);
        AssignFile(F, S);
        ReWrite(F, 1);
        BlockWrite(F, P^, FR[I].Size);
        CloseFile(F);
        FreeMem(P, FR[I].Size);
        WriteLn;
      End;
  SetLength(FR, 0);
  CloseFile(Fl);
End.

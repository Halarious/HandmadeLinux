
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

char* ReadEntireFileIntoMemoryAndNullTerminate(char* Filename)
{
  char* Result = 0;

  FILE* File = fopen(Filename, "r");
  if(File)
    {
      fseek(File, 0, SEEK_END);
      size_t FileSize = ftell(File);
      fseek(File, 0, SEEK_SET);

      Result = (char*) malloc(FileSize + 1);
      fread(Result, FileSize, 1, File);
      Result[FileSize] = 0;
      
      fclose(File);
    }

  return(Result);
}

typedef enum 
  {
    Token_Unknown,
    
    Token_OpenParen,
    Token_CloseParen,
    Token_Colon,
    Token_Semicolon,
    Token_Asterisk,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_OpenBrace,
    Token_CloseBrace,

    Token_String,
    Token_Identifier,

    Token_EndOfStream,

    Token_Count,
  } token_type;

typedef struct
{
  token_type Type;
  
  size_t Length;
  char* Text;
} token;

typedef struct
{
  char* At;
} tokenizer;

static inline bool
IsEndOfLine(char C)
{
  bool Result = ((C == '\n') ||
		 (C == '\r'));
  return(Result);
}

static inline bool
IsWhitespace(char C)
{
  bool Result = ((C == ' ')  ||
		 (C == '\t') ||
		 (C == '\v') ||
		 (C == '\f') ||
		 IsEndOfLine(C));
  return(Result);
}

static inline bool
IsAlpha(char C)
{
  bool Result = (((C >= 'a') && (C <= 'z')) ||
		 ((C >= 'A') && (C <= 'Z')));
  return(Result);
}

static inline bool
IsNumber(char C)
{
  bool Result = ((C >= '0') && (C <= '9'));
  return(Result);
}

static inline bool
TokenEquals(token Token, char* Match)
{
  char* At = Match;
  for(int Index = 0;
      Index < Token.Length;
      ++Index, ++At)
    {
      if((*At == '\0') ||
	 (Token.Text[Index] != *At))
	{
	  return(false);
	}
    }

  bool Result = (*At == '\0');
  return(Result);
}

static void
EatAllWhitespace(tokenizer* Tokenizer)
{
  for(;;)
    {
      if(IsWhitespace(Tokenizer->At[0]))
	{
	  ++Tokenizer->At;
	}
      else if((Tokenizer->At[0] == '/') &&
	      (Tokenizer->At[1] == '/'))
	{
	  Tokenizer->At += 2;
	  while((Tokenizer->At[0]             ) &&
		(!IsEndOfLine(Tokenizer->At[0])))
	    {
	      ++Tokenizer->At;
	    }
	}
      else if((Tokenizer->At[0] == '/') &&
	      (Tokenizer->At[1] == '*'))
	{
	  Tokenizer->At += 2;
	  while((Tokenizer->At[0]       ) &&
		!((Tokenizer->At[0] != '*') &&
		  (Tokenizer->At[1] != '/')))
	    {
	      ++Tokenizer->At;
	    }

	  if(Tokenizer->At[0] == '*')
	    {
	      Tokenizer->At += 2;	      
	    }
	}
      else
	{
	  break;
	}
    }
}

static token
GetToken(tokenizer* Tokenizer)
{
  EatAllWhitespace(Tokenizer);
  
  token Token  = {};
  Token.Length = 1;
  Token.Text   = Tokenizer->At;
  char C = Tokenizer->At[0];
  ++Tokenizer->At;
  switch(C)
    {
    case('\0'):{Token.Type = Token_EndOfStream;}   break;

    case('('): {Token.Type = Token_OpenParen;}     break;
    case(')'): {Token.Type = Token_CloseParen;}    break;
    case(':'): {Token.Type = Token_Colon;}         break;
    case(';'): {Token.Type = Token_Semicolon;}     break;
    case('*'): {Token.Type = Token_Asterisk;}      break;
    case('['): {Token.Type = Token_OpenBracket;}   break;
    case(']'): {Token.Type = Token_CloseBracket;}  break;
    case('{'): {Token.Type = Token_OpenBrace;}     break;
    case('}'): {Token.Type = Token_CloseBrace;}    break;
      
    case('"'):
      {
	Token.Type = Token_String;
	Token.Text = Tokenizer->At;
		
	while(Tokenizer->At[0]  && 
	      Tokenizer->At[0] != '"')
	  {
	    if((Tokenizer->At[0] == '\\') &&
	       (Tokenizer->At[1]))
	      {
		++Tokenizer->At;
	      }
	    ++Tokenizer->At;
	  }

	Token.Length = Tokenizer->At - Token.Text;
	if(Tokenizer->At[0] == '"')
	  {
	    ++Tokenizer->At;
	  }
      } break;

    default:
      {
	if(IsAlpha(C))
	  {
	    Token.Type = Token_Identifier;
	    while((IsAlpha( Tokenizer->At[0])) ||
		  (IsNumber(Tokenizer->At[0])) ||
		  (Tokenizer->At[0] == '_'))
	      {
		++Tokenizer->At;
	      }

	    Token.Length = Tokenizer->At - Token.Text;
	  }
#if 0
	else if(IsNumber(C))
	  {
	    ParseNumber();
	  }
#endif
	else
	  {
	    Token.Type = Token_Unknown; 
	  }
      } break;
    }

  return(Token);
}

static bool
RequireToken(tokenizer* Tokenizer, token_type DesiredType)
{
  token Token = GetToken(Tokenizer);

  bool Result = (Token.Type == DesiredType);
  return(Result);
}

static void
ParseIntrospectionParams(tokenizer* Tokenizer)
{
  for(;;)
    {
      token Token = GetToken(Tokenizer);
      if((Token.Type == Token_CloseParen) ||
	 (Token.Type == Token_EndOfStream))
	{
	  break;
	}
    }
}

static void
ParseMember(tokenizer* Tokenizer, token MemberTypeToken)
{
#if 1
  bool Parsing = true;
  bool IsPointer = false;
  while(Parsing)
    {
      token Token = GetToken(Tokenizer);
      switch(Token.Type)
	{
	case(Token_Asterisk):
	  {
	    IsPointer = true;
	  } break;

	case(Token_Identifier):
	  {
	    printf("{Type_%.*s, \"%.*s\"},\n",
		   (int)MemberTypeToken.Length, MemberTypeToken.Text,
		   (int)Token.Length, Token.Text);
	  } break;
	  
	case(Token_Semicolon):
	case(Token_EndOfStream):
	{
	  Parsing = false;
	} break;

	default:
	  {
	    
	  }
	}
    }
#else
  token Token = GetToken(Tokenizer);
  switch(Token.Type)
    {
    case(Token_Asterisk):
      {
	IsPointer = true;
	ParseMember(Tokenizer, Token);
      } break;

    case(Token_Identifier):
      {
	printf("DEBUG_VALUE(%.*s);\n", (int)Token.Length, Token.Text);
      } break;
      
    default:
      {
	    
      }
    }
#endif
}

static void
ParseStruct(tokenizer* Tokenizer)
{
  token NameToken = GetToken(Tokenizer);
  if(RequireToken(Tokenizer, Token_OpenBrace))
    {
      printf("member_definition MembersOf_%.*s[] = \n", (int)NameToken.Length, NameToken.Text);
      printf("{\n");
      for(;;)
	{
	  token MemberToken = GetToken(Tokenizer);
	  if(MemberToken.Type == Token_CloseBrace)
	    {
	      break;
	    }
	  else
	    {
	      ParseMember(Tokenizer, MemberToken);
	    }
	}
      printf("}\n");
    }
}

static void
ParseIntrospectable(tokenizer* Tokenizer)
{
  if(RequireToken(Tokenizer, Token_OpenParen))
    {
      ParseIntrospectionParams(Tokenizer);

      token TypeToken = GetToken(Tokenizer);
      if(TokenEquals(TypeToken, "struct"))
	{
	  ParseStruct(Tokenizer);
	}
      else
	{
	  fprintf(stderr, "ERROR: Introspection is only supported for struct right now :(.\n");
	}
    }
  else
    {
      fprintf(stderr, "ERROR: Missing paren.\n");
    }
}

int main(int ArgC, char** Args)
{
  char* FileContents = ReadEntireFileIntoMemoryAndNullTerminate("../code/handmade_sim_region.h");

  tokenizer Tokenizer = {};
  Tokenizer.At = FileContents;
  
  bool Parsing = true;
  while(Parsing)
    {
      token Token = GetToken(&Tokenizer);
      switch(Token.Type)
	{
	  
	case(Token_EndOfStream):
	  {
	    Parsing = false;
	  } break;
	  
	case(Token_Unknown):
	  {
	    
	  } break;
	  
	case(Token_Identifier):
	  {
	    if(TokenEquals(Token, "introspect"))
	      {
		ParseIntrospectable(&Tokenizer);
	      }
	  } break;

	default:
	  {
	    //printf("%d: %.*s\n", Token.Type, (int)Token.Length, Token.Text);
	  } break;
	}
    }
  
  return(0);
}


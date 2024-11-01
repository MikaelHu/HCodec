#pragma once


#define SAFE_DEL(p) {if(p){delete (p); (p) = nullptr;}}
#define SAFE_DEL_A(p) {if(p){delete[] (p); (p) = nullptr;}}

#ifndef RETURN_IF
#define RETURN_IF(p) do{if(!(p)) return;}while(0)
#endif

#ifndef RETURN_V_IF
#define RETURN_V_IF(p, v) do{if(!(p)) return(v);}while(0)
#endif
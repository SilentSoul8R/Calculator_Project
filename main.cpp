#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

// ─── ANSI escape helpers ───────────────────────────────────────────
#define ESC "\033["
#define CLEAR_SCREEN()   std::cout << ESC "2J" ESC "H"
#define HIDE_CURSOR()    std::cout << ESC "?25l"
#define SHOW_CURSOR()    std::cout << ESC "?25h"
#define MOVE(r,c)        std::cout << ESC << (r) << ";" << (c) << "H"
#define RESET_ATTR()     std::cout << ESC "0m"
#define BOLD_ON()        std::cout << ESC "1m"

#define FG(r,g,b) std::cout<<ESC"38;2;"<<(r)<<";"<<(g)<<";"<<(b)<<"m"
#define BG(r,g,b) std::cout<<ESC"48;2;"<<(r)<<";"<<(g)<<";"<<(b)<<"m"

struct RGB { int r,g,b; };

// ── Palette ────────────────────────────────────────────────────────
constexpr RGB BG_SHELL   = {14,  20,  40 };
constexpr RGB BG_DISPLAY = {8,   14,  30 };
constexpr RGB BG_NUM     = {22,  32,  60 };
constexpr RGB BG_SCI     = {16,  26,  52 };
constexpr RGB BG_OP      = {20,  45,  90 };
constexpr RGB BG_CLR     = {55,  18,  18 };
constexpr RGB BG_EQ      = {30,  80,  180};
constexpr RGB BG_SEL     = {60,  140, 255};

constexpr RGB FG_WHITE   = {220, 235, 255};
constexpr RGB FG_CYAN    = {100, 200, 255};
constexpr RGB FG_DIM     = {80,  110, 160};
constexpr RGB FG_RED     = {255, 100, 100};
constexpr RGB FG_GREEN   = {100, 220, 150};
constexpr RGB FG_DARK    = {10,  20,  40 };
constexpr RGB FG_BLUE_LT = {120, 180, 255};

inline void setFG(RGB c){ FG(c.r,c.g,c.b); }
inline void setBG(RGB c){ BG(c.r,c.g,c.b); }

// ─── Raw terminal ──────────────────────────────────────────────────
struct RawMode {
    termios orig;
    RawMode() {
        tcgetattr(STDIN_FILENO, &orig);
        termios raw = orig;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN]  = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }
    ~RawMode() {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
        SHOW_CURSOR();
        RESET_ATTR();
        std::cout << "\n";
    }
};

int term_cols() {
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    return ws.ws_col > 0 ? ws.ws_col : 80;
}
int term_rows() {
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    return ws.ws_row > 0 ? ws.ws_row : 24;
}

// ─── Key reading ──────────────────────────────────────────────────
enum Key { K_UP=256,K_DOWN,K_LEFT,K_RIGHT,K_ENTER,K_BACKSPACE,K_ESC,K_OTHER };
int read_key() {
    unsigned char c;
    if (read(STDIN_FILENO,&c,1)!=1) return K_OTHER;
    if (c=='\n'||c=='\r') return K_ENTER;
    if (c==127||c==8)     return K_BACKSPACE;
    if (c==27) {
        unsigned char seq[3]={};
        if (read(STDIN_FILENO,&seq[0],1)!=1) return K_ESC;
        if (seq[0]=='[') {
            read(STDIN_FILENO,&seq[1],1);
            if (seq[1]=='A') return K_UP;
            if (seq[1]=='B') return K_DOWN;
            if (seq[1]=='C') return K_RIGHT;
            if (seq[1]=='D') return K_LEFT;
        }
        return K_ESC;
    }
    return (int)c;
}

// ─── Expression parser ─────────────────────────────────────────────
const double kPI = M_PI, kE = M_E;

class Parser {
    std::string s;
    size_t p = 0;

    void skip() { while(p<s.size()&&s[p]==' ')p++; }

    double primary() {
        skip();
        if (p>=s.size()) throw std::runtime_error("Unexpected end");
        if (s[p]=='-') { p++; return -primary(); }
        if (s[p]=='+') { p++; return  primary(); }
        if (s[p]=='(') {
            p++;
            double v=expr_add();
            skip();
            if (p<s.size()&&s[p]==')') p++;
            else throw std::runtime_error("Missing ')'");
            return v;
        }
        if (isalpha(s[p])) {
            std::string name;
            while (p<s.size()&&isalpha(s[p])) name+=s[p++];
            if (name=="pi"||name=="PI") return kPI;
            if (name=="e"||name=="E")  return kE;
            skip();
            if (p>=s.size()||s[p]!='(')
                throw std::runtime_error("Expected '(' after "+name);
            p++;
            double arg=expr_add();
            skip();
            if (p<s.size()&&s[p]==')') p++;
            else throw std::runtime_error("Missing ')' for "+name);
            if(name=="sin")  return std::sin(arg);
            if(name=="cos")  return std::cos(arg);
            if(name=="tan")  return std::tan(arg);
            if(name=="asin") return std::asin(arg);
            if(name=="acos") return std::acos(arg);
            if(name=="atan") return std::atan(arg);
            if(name=="sqrt"){ if(arg<0)throw std::runtime_error("sqrt of neg"); return std::sqrt(arg);}
            if(name=="cbrt") return std::cbrt(arg);
            if(name=="log"||name=="log10"){if(arg<=0)throw std::runtime_error("log domain");return std::log10(arg);}
            if(name=="ln")  {if(arg<=0)throw std::runtime_error("ln domain"); return std::log(arg);}
            if(name=="abs")  return std::abs(arg);
            if(name=="exp")  return std::exp(arg);
            if(name=="ceil") return std::ceil(arg);
            if(name=="floor")return std::floor(arg);
            if(name=="round")return std::round(arg);
            throw std::runtime_error("Unknown: "+name);
        }
        if (isdigit(s[p])||s[p]=='.') {
            size_t start=p;
            while(p<s.size()&&(isdigit(s[p])||s[p]=='.'))p++;
            if(p<s.size()&&(s[p]=='e'||s[p]=='E')){
                p++;
                if(p<s.size()&&(s[p]=='+'||s[p]=='-'))p++;
                while(p<s.size()&&isdigit(s[p]))p++;
            }
            return std::stod(s.substr(start,p-start));
        }
        throw std::runtime_error(std::string("Unexpected: ")+s[p]);
    }

    double expr_pow() {
        double base=primary();
        skip();
        while(p<s.size()&&s[p]=='^'){
            p++;
            double exp=primary();
            base=std::pow(base,exp);
            skip();
        }
        return base;
    }

    double expr_mul() {
        double v=expr_pow();
        skip();
        while(p<s.size()&&(s[p]=='*'||s[p]=='/'||s[p]=='%')){
            char op=s[p++];
            double r=expr_pow();
            if(op=='*')v*=r;
            else if(op=='/'){if(r==0)throw std::runtime_error("Div by zero");v/=r;}
            else{if(r==0)throw std::runtime_error("Mod by zero");v=std::fmod(v,r);}
            skip();
        }
        return v;
    }

    double expr_add() {
        double v=expr_mul();
        skip();
        while(p<s.size()&&(s[p]=='+'||s[p]=='-')){
            char op=s[p++];
            double r=expr_mul();
            if(op=='+')v+=r; else v-=r;
            skip();
        }
        return v;
    }

public:
    double parse(const std::string& input) {
        s=input; p=0;
        double v=expr_add();
        skip();
        if(p!=s.size())
            throw std::runtime_error("Unexpected near pos "+std::to_string(p));
        return v;
    }
};

std::string fmt(double v) {
    if(std::isinf(v)) return v>0?"Infinity":"-Infinity";
    if(std::isnan(v)) return "Undefined";
    if(v==std::floor(v)&&std::abs(v)<1e15){
        std::ostringstream o; o<<std::fixed<<std::setprecision(0)<<v; return o.str();
    }
    std::ostringstream o;
    o<<std::setprecision(12)<<v;
    std::string s=o.str();
    if(s.find('.')!=std::string::npos&&s.find('e')==std::string::npos){
        s.erase(s.find_last_not_of('0')+1);
        if(s.back()=='.') s.pop_back();
    }
    return s;
}

// ─── Button definitions ────────────────────────────────────────────
enum BtnType { T_NUM,T_SCI,T_OP,T_CLR,T_EQ };
struct Btn { std::string lbl,ins,act; BtnType type; int span; };

std::vector<std::vector<Btn>> GRID = {
    {{"sin",  "sin(",  "", T_SCI,1},{"cos","cos(","",T_SCI,1},
     {"tan",  "tan(",  "", T_SCI,1},{"log","log(","",T_SCI,1},{"ln","ln(","",T_SCI,1}},
    {{"asin", "asin(", "", T_SCI,1},{"acos","acos(","",T_SCI,1},
     {"atan", "atan(", "", T_SCI,1},{"sqrt","sqrt(","",T_SCI,1},{"x^y","^","",T_SCI,1}},
    {{"pi",   "pi",    "", T_SCI,1},{"e","e","",T_SCI,1},
     {"( )",  "",  "paren",T_SCI,1},{"%","%","",T_SCI,1},{"1/x","","recip",T_SCI,1}},
    {{"AC",   "",  "clear",T_CLR,1},{"<-","","back",T_CLR,1},
     {"+/-",  "", "negate",T_OP, 1},{"÷", "/","",T_OP,2}},
    {{"7","7","",T_NUM,1},{"8","8","",T_NUM,1},{"9","9","",T_NUM,1},{"×","*","",T_OP,2}},
    {{"4","4","",T_NUM,1},{"5","5","",T_NUM,1},{"6","6","",T_NUM,1},{"-","-","",T_OP,2}},
    {{"1","1","",T_NUM,1},{"2","2","",T_NUM,1},{"3","3","",T_NUM,1},{"+","+","",T_OP,2}},
    {{"0","0","",T_NUM,2},{".",".",  "",T_NUM,1},{"=","","eval",T_EQ,2}},
};
const int ROWS=(int)GRID.size();

// ─── Calculator state ──────────────────────────────────────────────
struct Calc {
    std::string expr,result,hist;
    bool error=false,rad=true;
    int sel_row=0,sel_col=0;
    Parser parser;

    void clear() { expr="";result="";hist="";error=false; }
    void back()  { if(!expr.empty())expr.pop_back(); error=false; }

    void ins(const std::string& s){
        if(error){expr=s;error=false;}
        else expr+=s;
    }
    void paren(){
        int o=(int)std::count(expr.begin(),expr.end(),'(');
        int c=(int)std::count(expr.begin(),expr.end(),')');
        ins(o>c?")":"(");
    }
    void recip(){ if(!expr.empty()) expr="1/("+expr+")"; }
    void negate(){
        if(expr.empty()){expr="-";return;}
        if(expr.size()>3&&expr[0]=='-'&&expr[1]=='(')
            expr=expr.substr(2,expr.size()-3);
        else if(expr[0]=='-') expr=expr.substr(1);
        else expr="-("+expr+")";
    }
    void eval(){
        if(expr.empty())return;
        std::string e=expr;
        if(!rad){
            for(auto& fn:{"sin","cos","tan","asin","acos","atan"}){
                std::string from=std::string(fn)+"(";
                std::string to  =std::string(fn)+"(pi/180*";
                size_t pos=0;
                while((pos=e.find(from,pos))!=std::string::npos){e.replace(pos,from.size(),to);pos+=to.size();}
            }
        }
        try {
            double v=parser.parse(e);
            hist=expr+" =";
            result=fmt(v);
            expr=result;
            error=false;
        } catch(const std::exception&){
            hist=expr;
            result="Error";
            error=true;
        }
    }
    void press(const Btn& b){
        if(!b.act.empty()){
            if(b.act=="clear") clear();
            else if(b.act=="back")   back();
            else if(b.act=="paren")  paren();
            else if(b.act=="recip")  recip();
            else if(b.act=="negate") negate();
            else if(b.act=="eval")   eval();
        } else ins(b.ins);
    }
};

// ─── Drawing ───────────────────────────────────────────────────────
const int SHELL_W = 56;
const int BTN_W   = 10;
const int BTN_H   = 3;
const int DISP_H  = 8;
const int SHELL_H = DISP_H + 3 + ROWS*BTN_H + 4;

void fill(int row,int col,int h,int w,RGB fg,RGB bg){
    setFG(fg);setBG(bg);
    for(int r=0;r<h;r++){
        MOVE(row+r,col);
        for(int i=0;i<w;i++) std::cout<<' ';
    }
    RESET_ATTR();
}

void rtext(int row,int col,int w,const std::string& s,RGB fg,RGB bg,bool bold=false){
    setBG(bg);setFG(fg);
    if(bold) BOLD_ON();
    MOVE(row,col);
    for(int i=0;i<w;i++) std::cout<<' ';
    std::string t=s;
    if((int)t.size()>w) t=t.substr(t.size()-w);
    MOVE(row,col+w-(int)t.size());
    std::cout<<t;
    RESET_ATTR();
}

void ctext(int row,int col,int w,const std::string& s,RGB fg,RGB bg,bool bold=false){
    setBG(bg);setFG(fg);
    if(bold) BOLD_ON();
    MOVE(row,col);
    for(int i=0;i<w;i++) std::cout<<' ';
    int px=col+(w-(int)s.size())/2;
    MOVE(row,px);
    std::cout<<s;
    RESET_ATTR();
}

RGB get_btn_bg(BtnType t,bool sel){
    if(sel) return BG_SEL;
    switch(t){
        case T_SCI: return BG_SCI;
        case T_NUM: return BG_NUM;
        case T_OP:  return BG_OP;
        case T_CLR: return BG_CLR;
        case T_EQ:  return BG_EQ;
    }
    return BG_NUM;
}
RGB get_btn_fg(BtnType t,bool sel){
    if(sel) return FG_DARK;
    switch(t){
        case T_SCI: return FG_CYAN;
        case T_NUM: return FG_WHITE;
        case T_OP:  return FG_BLUE_LT;
        case T_CLR: return FG_RED;
        case T_EQ:  return FG_WHITE;
    }
    return FG_WHITE;
}

void draw(const Calc& c, int oy, int ox){
    // Shell
    for(int r=0;r<SHELL_H;r++) fill(oy+r,ox,1,SHELL_W,FG_DIM,BG_SHELL);

    // Top border line
    MOVE(oy,ox);
    setFG(FG_DIM);setBG(BG_SHELL);
    std::cout<<"  ";
    setFG(FG_CYAN);
    for(int i=0;i<SHELL_W-4;i++) std::cout<<"\xe2\x94\x80"; // ─
    RESET_ATTR();

    // Brand
    MOVE(oy+1,ox+3);
    setFG(FG_CYAN);setBG(BG_SHELL);BOLD_ON();
    std::cout<<"SciCalc";
    RESET_ATTR();
    MOVE(oy+1,ox+10);
    setFG(FG_DIM);setBG(BG_SHELL);
    std::cout<<"Pro";
    RESET_ATTR();

    // Mode badge
    MOVE(oy+1,ox+SHELL_W-8);
    setFG(FG_GREEN);setBG(BG_SHELL);BOLD_ON();
    std::cout<<(c.rad?"[RAD]":"[DEG]");
    RESET_ATTR();

    // Display area
    int dt=oy+2, dl=ox+2, dw=SHELL_W-4;
    for(int r=0;r<DISP_H;r++) fill(dt+r,dl,1,dw,FG_WHITE,BG_DISPLAY);

    // History
    if(!c.hist.empty()){
        std::string h=c.hist;
        if((int)h.size()>dw-2) h=h.substr(h.size()-dw+2);
        MOVE(dt+1,dl+dw-1-(int)h.size());
        setFG(FG_DIM);setBG(BG_DISPLAY);
        std::cout<<h;
        RESET_ATTR();
    }

    // Separator inside display
    MOVE(dt+3,dl+1);
    setFG(FG_DIM);setBG(BG_DISPLAY);
    for(int i=0;i<dw-2;i++) std::cout<<"\xe2\x94\x80";
    RESET_ATTR();

    // Main value
    RGB num_color = c.error ? FG_RED : FG_CYAN;
    rtext(dt+5,dl+1,dw-2, c.expr.empty()?"0":c.expr, num_color, BG_DISPLAY, true);

    // Divider between sci rows and num rows
    int div_y = oy+2+DISP_H+1 + 3*BTN_H;
    MOVE(div_y,ox+2);
    setFG(FG_DIM);setBG(BG_SHELL);
    for(int i=0;i<SHELL_W-4;i++) std::cout<<"\xe2\x94\x80";
    RESET_ATTR();

    // Buttons
    int gt=oy+2+DISP_H+1, gl=ox+2;
    for(int row=0;row<ROWS;row++){
        int cx=0;
        for(int col=0;col<(int)GRID[row].size();col++){
            const Btn& b=GRID[row][col];
            int bx=gl+cx*BTN_W;
            int by=gt+row*BTN_H;
            int bw=b.span*BTN_W-1;
            bool sel=(row==c.sel_row&&col==c.sel_col);
            RGB bg=get_btn_bg(b.type,sel);
            RGB fg=get_btn_fg(b.type,sel);
            for(int r=0;r<BTN_H-1;r++) fill(by+r,bx,1,bw,fg,bg);
            ctext(by+(BTN_H-1)/2,bx,bw,b.lbl,fg,bg,sel||b.type==T_EQ);
            cx+=b.span;
        }
    }

    // Bottom border
    MOVE(oy+SHELL_H-2,ox+2);
    setFG(FG_DIM);setBG(BG_SHELL);
    for(int i=0;i<SHELL_W-4;i++) std::cout<<"\xe2\x94\x80";
    RESET_ATTR();

    // Footer hint
    MOVE(oy+SHELL_H-1,ox+3);
    setFG(FG_DIM);setBG(BG_SHELL);
    std::cout<<"arrows+enter | 0-9 +-*/^() m=mode q=quit";
    RESET_ATTR();

    std::cout.flush();
}

// ─── Main ──────────────────────────────────────────────────────────
int main(){
    HIDE_CURSOR();
    CLEAR_SCREEN();
    RawMode raw;
    Calc c;

    auto pos=[&](int& oy,int& ox){
        oy=std::max(1,(term_rows()-SHELL_H)/2);
        ox=std::max(1,(term_cols()-SHELL_W)/2);
    };

    int oy,ox; pos(oy,ox);

    while(true){
        CLEAR_SCREEN();
        draw(c,oy,ox);
        int k=read_key();
        if(k=='q'||k=='Q'||k==K_ESC) break;
        if(k=='m'||k=='M'){ c.rad=!c.rad; continue; }
        if(k==K_UP){   c.sel_row=std::max(0,c.sel_row-1); c.sel_col=std::min(c.sel_col,(int)GRID[c.sel_row].size()-1); }
        else if(k==K_DOWN){ c.sel_row=std::min(ROWS-1,c.sel_row+1); c.sel_col=std::min(c.sel_col,(int)GRID[c.sel_row].size()-1); }
        else if(k==K_LEFT)  c.sel_col=std::max(0,c.sel_col-1);
        else if(k==K_RIGHT) c.sel_col=std::min((int)GRID[c.sel_row].size()-1,c.sel_col+1);
        else if(k==K_ENTER) c.press(GRID[c.sel_row][c.sel_col]);
        else if(k==K_BACKSPACE) c.back();
        else if(k>='0'&&k<='9') c.ins(std::string(1,(char)k));
        else if(k=='.')  c.ins(".");
        else if(k=='+')  c.ins("+");
        else if(k=='-')  c.ins("-");
        else if(k=='*')  c.ins("*");
        else if(k=='/')  c.ins("/");
        else if(k=='^')  c.ins("^");
        else if(k=='(')  c.ins("(");
        else if(k==')')  c.ins(")");
        else if(k=='%')  c.ins("%");
        else if(k=='s')  c.ins("sin(");
        else if(k=='c')  c.ins("cos(");
        else if(k=='t')  c.ins("tan(");
        else if(k=='l')  c.ins("log(");
        else if(k=='n')  c.ins("ln(");
        else if(k=='r')  c.ins("sqrt(");
        else if(k=='p')  c.ins("pi");
        else if(k=='e')  c.ins("e");
        else if(k=='a')  c.clear();
        else if(k=='=')  c.eval();
        pos(oy,ox);
    }

    CLEAR_SCREEN();
    MOVE(1,1);
    if(!c.result.empty()&&!c.error)
        std::cout<<"Last result: "<<c.result<<"\n";
    return 0;
}

//
// Created by BraxtonN on 2/6/2017.
//

#ifndef SHARP_OBJECTDELCIRATION_H
#define SHARP_OBJECTDELCIRATION_H

#include "../../stdimports.h"
#include <sstream>

class RuntimeNote {

public:
    RuntimeNote(string file, string line, int l, int col)
    :
            file(file),
            l(line),
            line(l),
            col(col)
    {
    }

    string getNote(string message) {
        stringstream note;
        note << "in file: ";
        note << file << ":" << line << ":" << col << ": note:  " << message
                  << endl;

        note << '\t' << '\t' << l << endl << '\t' << '\t';

        for(int i = 0; i < col-1; i++)
            note << " ";
        note << "^" << endl;

        return note.str();
    }

    int getLine() { return line; }
    int getCol() { return col; }

private:
    string file;
    string l;
    int line,col;
};


#endif //SHARP_OBJECTDELCIRATION_H

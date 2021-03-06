/* esco - interpreter for a lot (in future) esoteric languages.
 * Copyright (C) 2007-2011, Zasenko Sergey
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <config.h>
#include <cstring>
#include "ookpreprocessor.h"
#include "bfmachine.h"
#include "defs.h"

OokPreProcessor::OokPreProcessor(SourceReader* _sr) {
    error = ERROR_OK;
    tsrc = _sr->read();
    if (tsrc == NULL)
        error = _sr->getError();
}

OokPreProcessor::~OokPreProcessor() {
    delete[] tsrc;
}

Source* OokPreProcessor::process() {
    if (tsrc) {
        int tsize = strlen(tsrc);

        int j = 0;

        char op = '\00';
        int dbgB = 0;
        Source* first = NULL;
        Source* src = NULL;

        while(j+3 < tsize) {
            if (tsrc[j] == ' ' || tsrc[j] == '\n') j++;
            else if (!strncmp(&tsrc[j], "Ook", 3)) {
                //Command format: "Ook[!.?][\r\t\n\0]"
                //TODO: Check for separator after the command

                if (op == '\00') {
                    dbgB = j;
                    op = tsrc[j+3];
                }
                else {
                    char nop = tsrc[j+3];
                    int bop = -1;
                    switch (op) {
                        case '.':
                            if (nop == '.') //Ook. Ook.
                                bop = inc_ptr_data;
                            else if (nop == '!') //Ook. Ook!
                                bop = input_ptr_data;
                            else if (nop == '?') //Ook. Ook?
                                bop = inc_ptr;
                            break;

                        case '!':
                            if (nop == '!') //Ook! Ook!
                                bop = dec_ptr_data;
                            else if (nop == '?') //Ook! Ook?
                                bop = while_begin;
                            else if (nop == '.') //Ook! Ook.
                                bop = output_ptr_data;
                            break;

                        case '?':
                            if (nop == '.') //Ook? Ook.
                                bop = dec_ptr;
                            else if (nop == '!') //Ook? Ook!
                                bop = while_end;
                            break;
                    }
                    if (bop != -1) {
                        src = new Source(bop, 0, dbgB, j+4-dbgB, src);
                        if (first == NULL)
                            first = src;
                    }
                op = '\00';
                }
                j += 5; //"Ook[!.?] " - 5 symbols;
            }
            else
                j++;
        }

        src = first;
        /* Look thru cycles and bind [ and ] thru param arg */
        while (src != NULL ) {
            int op = src->getOpcode();
            if (op == while_begin) {

                int brkts = 1;
                Source* tmp = src->getNext();
                while (tmp != NULL) {
                    if (tmp->getOpcode() == while_end) {
                        --brkts;
                        if (brkts == 0)
                            break;
                    }
                    else if (tmp->getOpcode() == while_begin)
                        ++brkts;
                    tmp = tmp->getNext();
                }
                if (tmp==NULL) {
                    error = ERROR_SYNTAX;
                    return NULL;
                }
                src->setFlowCtrl(tmp->getNext());
                tmp->setFlowCtrl(src);
            }
            src = src->getNext();
        }

       return first;
    }
    return NULL;
}

int OokPreProcessor::getError() {
    return error;
}

const char* OokPreProcessor::getSource() {
    return tsrc;
}



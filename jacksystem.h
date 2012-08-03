#ifndef JACK_SYSTEM_H
#define JACK_SYSTEM_H

typedef struct ThreebandSplitter_t ThreebandSplitter;

void SetupJackSystem (ThreebandSplitter *t);
void CloseJackSystem ();

#endif

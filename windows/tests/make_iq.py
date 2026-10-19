"""Create synthetic CW + ITA2 IQ for an isolated SDR# test, never live history."""
import argparse
from pathlib import Path
import wave
import numpy as np

MORSE = {'C':'-.-.','Q':'--.-','D':'-..','E':'.','K':'-.-','1':'.----',
         'A':'.-','B':'-...','T':'-','S':'...','M':'--','V':'...-',
         'R':'.-.','I':'..','7':'--...','3':'...--'}
LETTERS = {'A':3,'B':25,'C':14,'D':9,'E':1,'F':13,'G':26,'H':20,'I':6,
           'J':11,'K':15,'L':18,'M':28,'N':12,'O':24,'P':22,'Q':23,'R':10,
           'S':5,'T':16,'U':7,'V':30,'W':19,'X':29,'Y':21,'Z':17,' ':4,'\r':8,'\n':2}
DIGITS = {'0':22,'1':23,'2':19,'3':1,'4':10,'5':16,'6':21,'7':7,'8':6,'9':24}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    if args.output.exists(): raise SystemExit('Output already exists; choose a fresh test filename')
    rate=192000;duration=50
    cw=[]
    def cw_run(key,seconds): cw.append((key,seconds))
    cw_run(False,.5)
    for _ in range(4):
        for word in 'CQ CQ DE K1ABC TEST MAVERICK 73'.split():
            for letter in word:
                for mark in MORSE[letter]: cw_run(True,.06*(3 if mark=='-' else 1));cw_run(False,.06)
                cw_run(False,.12)
            cw_run(False,.24)
        cw_run(False,.5)
    rtty=[(True,.5)]
    def code(value):
        rtty.append((False,1/45.45))
        for bit in range(5):rtty.append((bool(value&(1<<bit)),1/45.45))
        rtty.append((True,1.5/45.45))
    for _ in range(8):
        figures=False;code(31)
        for char in 'RYRYRYRY\r\nCQ CQ DE K1ABC K1ABC TEST MAVERICK 599 73\r\n':
            if char in DIGITS:
                if not figures:code(27);figures=True
                code(DIGITS[char])
            else:
                if figures:code(31);figures=False
                code(LETTERS[char])
    cw_ends=np.cumsum([v[1] for v in cw]);rtty_ends=np.cumsum([v[1] for v in rtty])
    cw_states=np.array([v[0] for v in cw]);rtty_states=np.array([v[0] for v in rtty])
    rng=np.random.default_rng(314159);phase=0.
    args.output.parent.mkdir(parents=True,exist_ok=True)
    with wave.open(str(args.output),'wb') as output:
        output.setnchannels(2);output.setsampwidth(2);output.setframerate(rate)
        for start in range(0,duration*rate,65536):
            t=np.arange(start,min(start+65536,duration*rate),dtype=np.float64)/rate
            ci=np.searchsorted(cw_ends,t);ri=np.searchsorted(rtty_ends,t)
            begins=np.where(ci==0,0,cw_ends[np.maximum(ci-1,0)])
            envelope=np.minimum(1,np.minimum((t-begins)/.005,(cw_ends[ci]-t)/.005))*cw_states[ci]
            frequency=np.where(rtty_states[ri],2585.,2415.)
            angles=phase+np.cumsum(2*np.pi*frequency/rate);phase=angles[-1]%(2*np.pi)
            iq=.28*envelope*np.exp(2j*np.pi*750*t)+.20*np.exp(1j*angles)
            iq+=.001*(rng.standard_normal(len(t))+1j*rng.standard_normal(len(t)))
            values=np.column_stack((iq.real,iq.imag));output.writeframesraw(np.clip(values*32767,-32768,32767).astype('<i2').tobytes())
    print('Synthetic 50-second IQ: center 14070000 Hz, CW dial 14070750 Hz, RTTY pair center 14072500 Hz')

if __name__=='__main__':main()

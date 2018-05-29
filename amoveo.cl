#define ROTRIGHT(a,b) (rotate((a),(uint)(32-b)))

#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

#define SHA256_F0o(x,y,z) (bitselect ((x), (y), ((x) ^ (z))))
#define SHA256_F1o(x,y,z) (bitselect ((z), (y), (x)))

#define FALTE(x,y,z,w) (SIG1(x) + y + SIG0(z) + w)

#define RUNDE_INNEN(a,b,c,d,e,f,g,h,x) {      \
  h += x + EP1(e) + SHA256_F1o(e,f,g);           \
  d += h;                                       \
  h = h + EP0(a) + SHA256_F0o(a,b,c);           \
}

__kernel void amoveo(__global unsigned char *in, __global unsigned char *out) {
        __private int ii = get_global_id(0);
	__private unsigned char *iic = (unsigned char*)&ii;
	__private uint m[14];

        for(int i=0; i<8; i++)
		 out[ii * 8 + i] = 0;

	// 55 Byte vorgabe einpflegen:
        m[0]  = (in[ 0] << 24) | (in[ 1] << 16) | (in[ 2] << 8) | (in[ 3]);
        m[1]  = (in[ 4] << 24) | (in[ 5] << 16) | (in[ 6] << 8) | (in[ 7]);
        m[2]  = (in[ 8] << 24) | (in[ 9] << 16) | (in[10] << 8) | (in[11]);
        m[3]  = (in[12] << 24) | (in[13] << 16) | (in[14] << 8) | (in[15]);
        m[4]  = (in[16] << 24) | (in[17] << 16) | (in[18] << 8) | (in[19]);
        m[5]  = (in[20] << 24) | (in[21] << 16) | (in[22] << 8) | (in[23]);
        m[6]  = (in[24] << 24) | (in[25] << 16) | (in[26] << 8) | (in[27]);
        m[7]  = (in[28] << 24) | (in[29] << 16) | (in[30] << 8) | (in[31]);
        m[8]  = (in[32] << 24) | (in[33] << 16) | (in[34] << 8) | (in[35]);
        m[9]  = 0;
        m[10] = (in[40] << 24) | (in[41] << 16) | (in[42] << 8) | (in[43]);
        m[11] = 0;
        m[12] = ii << 8;	// m13 kann optimal vorberechnet werden

        __private uint a, b, c, d, e, f, g, h, i, j;
        __private uint ap, bp, cp, dp, ep, fp, gp, hp;

	a = 0x6a09e667;
	b = 0xbb67ae85;
	c = 0x3c6ef372;
	d = 0xa54ff53a;
	e = 0x510e527f;
	f = 0x9b05688c;
	g = 0x1f83d9ab;
	h = 0x5be0cd19;

	RUNDE_INNEN(a, b, c, d, e, f, g, h, m[0] + 0x428a2f98);
	RUNDE_INNEN(h, a, b, c, d, e, f, g, m[1] + 0x71374491);
	RUNDE_INNEN(g, h, a, b, c, d, e, f, m[2] + 0xb5c0fbcf);
	RUNDE_INNEN(f, g, h, a, b, c, d, e, m[3] + 0xe9b5dba5);
	RUNDE_INNEN(e, f, g, h, a, b, c, d, m[4] + 0x3956c25b);
	RUNDE_INNEN(d, e, f, g, h, a, b, c, m[5] + 0x59f111f1);
	RUNDE_INNEN(c, d, e, f, g, h, a, b, m[6] + 0x923f82a4);
	RUNDE_INNEN(b, c, d, e, f, g, h, a, m[7] + 0xab1c5ed5);
	RUNDE_INNEN(a, b, c, d, e, f, g, h, m[8] + 0xd807aa98);
	RUNDE_INNEN(h, a, b, c, d, e, f, g, m[9] + 0x12835b01);
	RUNDE_INNEN(g, h, a, b, c, d, e, f, m[10] + 0x243185be);
	RUNDE_INNEN(f, g, h, a, b, c, d, e, m[11] + 0x550c7dc3);
	RUNDE_INNEN(e, f, g, h, a, b, c, d, m[12] + 0x72be5d74);

	// Verschieben: Phase 1
	ap = a;
	cp = c + 0x80deb1fe + EP1(h) + SHA256_F1o(h,a,b);
	dp = d;
	ep = e;
	fp = f;
	gp = g + cp;
	hp = h;
	cp += EP0(d) + SHA256_F0o(d,e,f);

	// Verschieben: Phase 2
	bp = b + 0x9bdc06a7;

	__private uint w0p, w1p, w2p, w3p, w4pp, w5p, w7p, w9p;

	// Verschieben: Phase 3
	w0p  = FALTE((uint)0, m[9], m[1], m[0]);
	w1p  = FALTE((uint)0x000001b8, m[10], m[2], m[1]);
	w2p  = FALTE(w0p, m[11], m[3], m[2]);
	w3p  = FALTE(w1p, m[12], m[4], m[3]);
	w4pp = SIG1(w2p) + SIG0(m[5]) + m[4];
	w5p  = FALTE(w3p, (uint)0, m[6], m[5]);
	w7p  = FALTE(w5p, w0p, m[8], m[7]);
	w9p  = FALTE(w7p, w2p, m[10], m[9]);
	
	// Verschieben: Phase 4
	m[6] += 0x000001b8 + SIG0(m[7]);
	m[8] += w1p;
	m[10]+= w3p;

	__private uint mx = SIG1(w9p) + SIG0(m[12]);

        for(m[13] = 0x80; m[13] < (1 << 23); m[13]+= 0x100) {
        	__private uint w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, wa, wb, wc, wd, we, wf;

		c = cp + m[13];
  		g = gp + m[13];                           

		b = bp + EP1(g) + SHA256_F1o(g,hp,ap);
  		f = fp + b;
		b += EP0(c) + SHA256_F0o(c,dp,ep);

  		a = ap + 0xc19bf32c + EP1(f) + SHA256_F1o(f,g,hp);
  		e = ep + a;
		a += EP0(b) + SHA256_F0o(b,c,dp);

		h = hp + 0xe49b69c1 + w0p + EP1(e) + SHA256_F1o(e,f,g);   
		d = dp + h;
		h += EP0(a) + SHA256_F0o(a,b,c);           

		RUNDE_INNEN(h, a, b, c, d, e, f, g, w1p + 0xefbe4786 ); 
		RUNDE_INNEN(g, h, a, b, c, d, e, f, w2p + 0x0fc19dc6 ); 
		RUNDE_INNEN(f, g, h, a, b, c, d, e, w3p + 0x240ca1cc ); 
		w4 = w4pp + m[13];
		RUNDE_INNEN(e, f, g, h, a, b, c, d, w4 + 0x2de92c6f ); 
		RUNDE_INNEN(d, e, f, g, h, a, b, c, w5p + 0x4a7484aa ); 
		w6 = SIG1(w4) + m[6];
		RUNDE_INNEN(c, d, e, f, g, h, a, b, w6 + 0x5cb0a9dc ); 
		RUNDE_INNEN(b, c, d, e, f, g, h, a, w7p + 0x76f988da ); 
		w8 = SIG1(w6) + m[8];
		RUNDE_INNEN(a, b, c, d, e, f, g, h, w8 + 0x983e5152 ); 
		RUNDE_INNEN(h, a, b, c, d, e, f, g, w9p + 0xa831c66d ); 
		wa = SIG1(w8) + m[10];
		RUNDE_INNEN(g, h, a, b, c, d, e, f, wa + 0xb00327c8 ); 
		wb = w4 + mx;
		RUNDE_INNEN(f, g, h, a, b, c, d, e, wb + 0xbf597fc7 ); 
		wc = SIG1(wa) + w5p + SIG0(m[13]) + m[12];
		RUNDE_INNEN(e, f, g, h, a, b, c, d, wc + 0xc6e00bf3 ); 
		wd = SIG1(wb) + w6 + m[13];
		RUNDE_INNEN(d, e, f, g, h, a, b, c, wd + 0xd5a79147 ); 
		we = SIG1(wc) + w7p + 0x706e0034;
		RUNDE_INNEN(c, d, e, f, g, h, a, b, we + 0x06ca6351 ); 
		wf = SIG1(wd) + w8 + SIG0(w0p) + 0x000001b8;
		RUNDE_INNEN(b, c, d, e, f, g, h, a, wf + 0x14292967 );
		w0 = FALTE(we, w9p, w1p, w0p);  
		RUNDE_INNEN(a, b, c, d, e, f, g, h, w0 + 0x27b70a85 ); 
		w1 = FALTE(wf, wa, w2p, w1p);  
		RUNDE_INNEN(h, a, b, c, d, e, f, g, w1 + 0x2e1b2138 ); 
		w2 = FALTE(w0, wb, w3p, w2p);  
		RUNDE_INNEN(g, h, a, b, c, d, e, f, w2 + 0x4d2c6dfc ); 
		w3 = FALTE(w1, wc, w4, w3p);  
		RUNDE_INNEN(f, g, h, a, b, c, d, e, w3 + 0x53380d13 ); 
		w4 = FALTE(w2, wd, w5p, w4); 
		RUNDE_INNEN(e, f, g, h, a, b, c, d, w4 + 0x650a7354 ); 
		w5 = FALTE(w3, we, w6, w5p);  
		RUNDE_INNEN(d, e, f, g, h, a, b, c, w5 + 0x766a0abb ); 
		w6 = FALTE(w4, wf, w7p, w6);  
		RUNDE_INNEN(c, d, e, f, g, h, a, b, w6 + 0x81c2c92e ); 
		w7 = FALTE(w5, w0, w8, w7p);  
		RUNDE_INNEN(b, c, d, e, f, g, h, a, w7 + 0x92722c85 ); 
		w8 = FALTE(w6, w1, w9p, w8);  
		RUNDE_INNEN(a, b, c, d, e, f, g, h, w8 + 0xa2bfe8a1 ); 
		w9 = FALTE(w7, w2, wa, w9p);  
		RUNDE_INNEN(h, a, b, c, d, e, f, g, w9 + 0xa81a664b ); 
		wa = FALTE(w8, w3, wb, wa);  
		RUNDE_INNEN(g, h, a, b, c, d, e, f, wa + 0xc24b8b70 ); 
		wb = FALTE(w9, w4, wc, wb);  
		RUNDE_INNEN(f, g, h, a, b, c, d, e, wb + 0xc76c51a3 ); 
		wc = FALTE(wa, w5, wd, wc);  
		RUNDE_INNEN(e, f, g, h, a, b, c, d, wc + 0xd192e819 ); 
		wd = FALTE(wb, w6, we, wd);  
		RUNDE_INNEN(d, e, f, g, h, a, b, c, wd + 0xd6990624 ); 
		we = FALTE(wc, w7, wf, we);  
		RUNDE_INNEN(c, d, e, f, g, h, a, b, we + 0xf40e3585 ); 
		wf = FALTE(wd, w8, w0, wf);  
		RUNDE_INNEN(b, c, d, e, f, g, h, a, wf + 0x106aa070 ); 
		w0 = FALTE(we, w9, w1, w0);  
		RUNDE_INNEN(a, b, c, d, e, f, g, h, w0 + 0x19a4c116 ); 
		w1 = FALTE(wf, wa, w2, w1);  
		RUNDE_INNEN(h, a, b, c, d, e, f, g, w1 + 0x1e376c08 ); 
		w2 = FALTE(w0, wb, w3, w2);  
		RUNDE_INNEN(g, h, a, b, c, d, e, f, w2 + 0x2748774c ); 
		w3 = FALTE(w1, wc, w4, w3);  
		RUNDE_INNEN(f, g, h, a, b, c, d, e, w3 + 0x34b0bcb5 ); 
		w4 = FALTE(w2, wd, w5, w4);  
		RUNDE_INNEN(e, f, g, h, a, b, c, d, w4 + 0x391c0cb3 ); 
		w5 = FALTE(w3, we, w6, w5);  
		RUNDE_INNEN(d, e, f, g, h, a, b, c, w5 + 0x4ed8aa4a ); 
		w6 = FALTE(w4, wf, w7, w6);  
		RUNDE_INNEN(c, d, e, f, g, h, a, b, w6 + 0x5b9cca4f ); 
		w7 = FALTE(w5, w0, w8, w7);  
		RUNDE_INNEN(b, c, d, e, f, g, h, a, w7 + 0x682e6ff3 ); 
		w8 = FALTE(w6, w1, w9, w8);  
		RUNDE_INNEN(a, b, c, d, e, f, g, h, w8 + 0x748f82ee ); 
		w9 = FALTE(w7, w2, wa, w9);  
		RUNDE_INNEN(h, a, b, c, d, e, f, g, w9 + 0x78a5636f ); 
		wa = FALTE(w8, w3, wb, wa);  
		RUNDE_INNEN(g, h, a, b, c, d, e, f, wa + 0x84c87814 ); 
		wb = FALTE(w9, w4, wc, wb);  
		RUNDE_INNEN(f, g, h, a, b, c, d, e, wb + 0x8cc70208 ); 
		wc = FALTE(wa, w5, wd, wc);  
		RUNDE_INNEN(e, f, g, h, a, b, c, d, wc + 0x90befffa ); 
		wd = FALTE(wb, w6, we, wd);  

		c += 0xa4506ceb + wd + EP1(h) + SHA256_F1o(h,a,b);
		g += c;                                       
		c += EP0(d) + SHA256_F0o(d,e,f);           
		b += 0xbef9a3f7 + FALTE(wc, w7, wf, we) + EP1(g) + SHA256_F1o(g,h,a);   
		f += b;
		b += EP0(c) + SHA256_F0o(c,d,e);           
  		a += FALTE(wd, w8, w0, wf) + EP1(f) + SHA256_F1o(f,g,h) + EP0(b) + SHA256_F0o(b,c,d);           

		if(a == 0xcf84a0a7) {
			unsigned char *mv = (unsigned char*)&m[12];

			out[ii * 8 + 0] = mv[3];
			out[ii * 8 + 1] = mv[2];
			out[ii * 8 + 2] = mv[1];
			out[ii * 8 + 3] = mv[0];

			mv = (unsigned char*)&m[13];

			out[ii * 8 + 4] = mv[3];
			out[ii * 8 + 5] = mv[2];
			out[ii * 8 + 6] = mv[1];

			mem_fence(CLK_GLOBAL_MEM_FENCE);
			return;
		}
        }
}


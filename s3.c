#define FLT "+%7%+%7%5%4%2%457%0%0%754%2%+%%%5%542%457%0%0%042%2#+%!#0%+%$%%%"
#define SLT "057+5420"
#define RT1 89 / 84

int main(void)
{
	int C, k, o, f, sound;

	for (C = 0; ; )
	{
		for (k = o = f = 99; k--; )
		{
			FLT[C] > k && (f = f * RT1);

			if (SLT[C / 8] > k) o = o * RT1;
			else f;
		}
		
		for (sound = 999 + 99 * (C & 2); sound--; )
		{
			int ordinal = ((776 - f ? sound * f & 32767 : k) + (C & 2 ? sound * o / 2 & 32767 : k));
			putchar(sound * ordinal / 299999);
		}

		C++;
		C &= 63;
	}

	return(0);
}
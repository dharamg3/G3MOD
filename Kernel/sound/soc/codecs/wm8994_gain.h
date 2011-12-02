
struct gain_info_t {
	int flag;
	int reg;
	int gain;
};

void wm8994_set_volume(struct snd_soc_codec *codec, unsigned short flag, unsigned short gain_reg, unsigned short gain_val);
int wm8994_get_gain_table_index(int codec_path);
void wm8994_set_default_gain(struct snd_soc_codec *codec, int codec_path);
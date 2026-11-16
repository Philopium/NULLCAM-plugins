# MAURI-MODE - installazione

## Cosa contiene lo zip

```
MAURI-MODE.vst3            il plugin
LEGGIMI - installazione.md  questo file
```

## Windows

1. Copia la cartella `MAURI-MODE.vst3` dentro una delle cartelle VST3:

   - `C:\Program Files\Common Files\VST3` (per tutti gli utenti)
   - `%LOCALAPPDATA%\Programs\Common\VST3` (solo per te, senza diritti di admin)

   Se la cartella `MAURI-MODE.vst3` esiste già, **sostituiscila** con quella nuova:
   non serve disinstallare nulla.

2. Riscansiona i plugin nel tuo DAW.

   - **Ableton Live**: `Preferenze > Plug-in > Riscansiona`. Se usi la cartella
     per-utente, aggiungi `%LOCALAPPDATA%\Programs\Common\VST3` in
     `Plug-in > "cartella VST3 personalizzata"`.
   - **Reaper**: `Opzioni > Preferenze > Plugin > VST: imposta la cartella VST3 e
     clicca "Aggiorna"`.
   - **Cubase / Logic / Studio One**: dovrebbe bastare aprire e chiudere la
     scansione dei plugin (o riavviare la DAW).

3. Cerca **MAURI-MODE** nella lista degli effetti e inseriscilo su una traccia.

## Primi passi

1. Metti `CLOCK` su **HOST** (il DAW passa il tempo) e `TIME SIG` su **4/4**.
2. `STYLE` su **GROOVE**, `SLICE` intorno al 60-70%.
3. Suona: piano ottieni un pulse sui tempi forti, più forte il pattern si riempie,
   fortissimo diventa uno shuffle con ratchet e reverse.
4. Se non sente niente, alza `LEVEL` e verifica `MIX` (deve essere 100%).

## Preset

I tuoi preset stanno in:

```
%APPDATA%\NULLCAM\MAURI-MODE\presets
```

Un file `.xml` per preset. Puoi copiarli, backed-up o mandarli a un amico: sono
file di testo e il percorso è leggibile.

Per cancellarli: il pulsante `DEL` nella barra in alto, oppure cancella il file.

## Problemi

**Non compare nella lista dei plugin.**
Controlla di aver copiato la cartella `MAURI-MODE.vst3` (cartella intera, con
`Contents` dentro) e riavvia/riscansiona la DAW.

**Suona solo se piccolo, il resto è muto.**
È il chop: il gate apre solo sui step attivi. Alza `MIX` e `LEVEL`, e verifica che
`CLOCK` non sia su `INPUT` senza suonare.

**Il ritmo non segue la DAW.**
Alcune DAW non mandano il BPM a un VST3 in tempo reale. In quel caso usa
`CLOCK = FREE` (120 BPM) oppure `CLOCK = INPUT`: in questo caso il ritmo lo
comanda il tuo playing.

**Il pattern resta sempre identico.**
`DYN PAT` sotto il 100% tiene il groove statico e lascia solo pochi gradi di
variazione. Alzalo al 100% e suona con più dinamica: il `SENS` decide quanto viene
letto il livello.

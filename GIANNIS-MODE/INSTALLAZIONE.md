# GIANNIS-MODE - installazione

## Cosa contiene lo zip

```
GIANNIS-MODE.vst3             il plugin
LEGGIMI - installazione.md    questo file
```

## Windows

1. Copia la cartella `GIANNIS-MODE.vst3` dentro una delle cartelle VST3:

   - `C:\Program Files\Common Files\VST3` (per tutti gli utenti)
   - `%LOCALAPPDATA%\Programs\Common\VST3` (solo per te, senza diritti di admin)

   Se la cartella `GIANNIS-MODE.vst3` esiste già, **sostituiscila** con quella nuova:
   non serve disinstallare nulla.

2. Riscansiona i plugin nel tuo DAW.

   - **Ableton Live**: `Preferenze > Plug-in > Riscansiona`. Se usi la cartella
     per-utente, aggiungi `%LOCALAPPDATA%\Programs\Common\VST3` in
     `Plug-in > "cartella VST3 personalizzata"`.
   - **Reaper**: `Opzioni > Preferenze > Plugin > VST: imposta la cartella VST3 e
     clicca "Aggiorna"`.
   - **Cubase / Studio One**: apri e chiudi la scansione dei plugin (o riavvia la DAW).

3. Cerca **GIANNIS-MODE** nella lista degli effetti e inseriscilo su una traccia.

## Primi passi

1. Metti il plugin su una traccia di basso o chitarra e suona una nota: senti subito
   la nota un'ottava sotto (`SUB`), più l'ottava sopra (`OCT`) se la alzi.
2. `DOOM` aggiunge due ottave sotto: è il sub più profondo, quello che serve per il
   dubstep. Alza `SUB` e `DOOM` insieme, poi aggiusta `FUZZ` e `TONE` per il carattere.
3. La secca è sempre presente a unità: il plugin non toglie mai il tuo suono originale,
   ci aggiunge le voci generate.
4. `TONE` scurisce o schiarisce solo la voce fuzzata, mai i sub: questi restano puliti
   e grandi anche a volume basso.
5. `LEVEL` a zero è muto: usalo per azzerare tutto senza toccare gli altri comandi.

Non serve nessun routing MIDI: l'ingresso audio della traccia è già l'informazione che
gli serve. Se la traccia ha un altro effetto davanti, il plugin ascolta quello.

## Preset

I tuoi preset stanno in:

```
%APPDATA%\NULLCAM\GIANNIS-MODE\presets
```

Un file `.xml` per preset, leggibile: puoi copiarlo, salvarlo o mandarlo a un amico.

Per cancellarli: il pulsante `DEL` nella barra in alto, oppure cancella il file.

## Problemi

**Non compare nella lista dei plugin.**
Controlla di aver copiato la cartella `GIANNIS-MODE.vst3` intera (con `Contents`
dentro) e riavvia/riscansiona la DAW.

**Non sento il sub.**
Suona una nota pulita: il divider ha bisogno di una nota con un'altezza definita.
Alza `SUB` e `DOOM`, e verifica che `LEVEL` non sia a zero.

**Il sub non parte sulla prima nota.**
È voluto: il divider parte al primo ciclo, ma ha bisogno di qualche millisecondo per
misurare il periodo. Suona una nota sostenuta e si assesta subito.

**Sembra troppo sporco.**
Abbassa `FUZZ` e alza `TONE`: i sub restano puliti comunque, solo la voce cambia.

## Licenza e codice sorgente

GIANNIS-MODE è software libero: puoi usarlo, copiarlo e modificarlo secondo la
**GNU Affero General Public License v3** (o una versione successiva). Il plugin è
distribuito **senza garanzia** di alcun tipo.

Il codice sorgente completo corrispondente a questo binario è pubblico qui:

```
https://github.com/Philopium/NULLCAM-plugins
```

(licenza completa: `https://www.gnu.org/licenses/agpl-3.0.html`)

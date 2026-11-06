# Logros nativos de Xbox 360

**Conclusión:** posible en RGH, sin herramientas de Microsoft, pero requiere trabajo en xsdk. Se hace al final (hito 9).
El sistema del juego (`src/game/achievements.cpp`, `src/app/achstore.cpp`) sigue siendo la fuente de verdad.

## Qué existe hoy

- 82 logros en el juego: 20 SMB1, 19 Lost Levels, 20 SMB2, 21 SMB3, 2 de la colección (`GameId::Boot`).
  Algunos tienen progreso (`target`) o son conjuntos (`set`).
- `xsdk-main/tools/elf2xex.py` ya incrusta un XDBF con el nombre del título (`title_database`, `add_title_name`).
- `xsdk-main/docs/PARITY.md`: Achievements = "No", "Through `XMsgStartIORequest` (xam_msg.cc); low priority".

## Trabajo necesario

1. **SPA/XDBF con logros**: ampliar el generador de XDBF (en el build del port o, acordándolo, en `elf2xex.py`) con
   `XACH` (id, cadenas, imagen, gamerscore, flags), cadenas por idioma en `XSTR` y las imágenes 64×64. Referencia del
   formato: Xenia, `src/xenia/kernel/util/xdbf_utils.*`.
2. **Tabla de correspondencia**: `src/platform/xenon/ach_map` con id del juego (`"smb1_warp"`) → id numérico nativo,
   generada desde `achievementDefs()` para que no se desincronicen.
3. **Desbloqueo**: `XamUserWriteAchievements` por `XMsgStartIORequest`, asíncrono con `xsdk_xam_*`; solo con perfil
   iniciado (`xsdk_xam_signed_in_user`). Al cargar, re-sincronizar los ya ganados en `achievements.ini`.
4. **Arte**: iconos propios (no extraídos de la ROM).

## Decisiones pendientes del usuario

- ~~Title ID definitivo~~: `0x534D4153`, "Super Mario All-Stars" (tarea 0021, `SuperMarioAllStars.xex`).
- Gamerscore: 1000 G repartidos o 0 G.
- Aviso: no conectar a Xbox Live un perfil con estos logros.

## Verificación

Xenia: solo humo (que no crashee al escribir). Consola: notificación, logro visible en el perfil, persistencia tras
reiniciar, comportamiento sin perfil.

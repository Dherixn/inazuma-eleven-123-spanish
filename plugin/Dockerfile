# Entorno para compilar el plugin 3GX: devkitARM + libctru (imagen oficial de devkitPro)
# + CTRPluginFramework y 3gxtool (repositorio de paquetes de ThePixellizerOSS).
FROM devkitpro/devkitarm:latest

RUN printf '\n[thepixellizeross-lib]\nServer = https://thepixellizeross.gitlab.io/packages/any\nSigLevel = Optional\n\n[thepixellizeross-linux]\nServer = https://thepixellizeross.gitlab.io/packages/x86_64/linux\nSigLevel = Optional\n' \
        >> /opt/devkitpro/pacman/etc/pacman.conf \
    && /opt/devkitpro/pacman/bin/pacman -Sy --noconfirm \
    && /opt/devkitpro/pacman/bin/pacman -S --noconfirm --needed libctrpf 3gxtool \
    && /opt/devkitpro/pacman/bin/pacman -Scc --noconfirm

ENV PATH="/opt/devkitpro/tools/bin:${PATH}"
WORKDIR /plugin

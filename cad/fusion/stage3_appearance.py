# Stage 3: appearances (colourway). Safe to re-run; change the colours here to try another colourway.

COLOURS = {
    "Digicam frame": ("Plastic - Matte (White)", (201, 184, 242)),     # lilac
    "Digicam accent": ("Plastic - Glossy (Red)", (255, 138, 61)),      # orange
}
BODY_LOOK = {
    "Frame": "Digicam frame",
    "Back plate (clear)": "Acrylic (Clear)",
    "Front plate (clear)": "Acrylic (Clear)",
    "Lens ring": "Digicam accent",
    "Viewfinder trims": "Digicam accent",
    "Button caps": "Digicam accent",
    "Shutter cap": "Digicam accent",
}
REF_LOOK = {
    "Battery": "Plastic - Matte (Gray)",
    "GOOUUU": "Plastic - Matte (Green)",
    "Screen module": "Plastic - Glossy (Black)",
    "Screen active": "Glass (Blue)",
    "Charge": "Plastic - Glossy (Blue)",
    "OV3660": "Plastic - Glossy (Black)",
    "switch": "Plastic - Matte (Black)",
}


def appearance(name):
    lib = app.materialLibraries.itemByName("Fusion Appearance Library")
    a = design.appearances.itemByName(name)
    if a:
        return a
    if name in COLOURS:
        base, rgb = COLOURS[name]
        a = design.appearances.addByCopy(lib.appearances.itemByName(base), name)
        prop = a.appearanceProperties.itemByName("Color")
        if prop:
            adsk.core.ColorProperty.cast(prop).value = adsk.core.Color.create(*rgb, 255)
        return a
    return design.appearances.addByCopy(lib.appearances.itemByName(name), name)


def main():
    for name, (base, rgb) in COLOURS.items():   # refresh colours if re-run
        a = appearance(name)
        prop = a.appearanceProperties.itemByName("Color")
        if prop:
            adsk.core.ColorProperty.cast(prop).value = adsk.core.Color.create(*rgb, 255)
    root = design.rootComponent
    done = []
    for i in range(root.occurrences.count):
        comp = root.occurrences.item(i).component
        for j in range(comp.bRepBodies.count):
            b = comp.bRepBodies.item(j)
            look = BODY_LOOK.get(comp.name)
            if comp.name.startswith("Reference"):
                look = next((v for k, v in REF_LOOK.items() if k in b.name), "Plastic - Matte (Gray)")
            if look:
                b.appearance = appearance(look)
                done.append(f"{comp.name}/{b.name}: {look}")
    print("\n".join(done))

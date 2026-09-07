# ErgoDox Custom External Holder & Mounting Specifications

This document outlines the exact coordinates, spacing, and dimensions for the custom 4-hole M3 mounting pattern integrated into the ErgoDox acrylic case files ([Layer 3 Plate](file:///c:/Users/Starlex/Documents/GitHub/ErgoDox/ErgoDox%20Acrylic%20Case/ErgoDOX%20Acrylic%20Case%20-%20Custom/Left/ErgoDOX%20Case%20Layer%203%20Plate%20-%20Left.dxf) and [Layer 5 Bottom](file:///c:/Users/Starlex/Documents/GitHub/ErgoDox/ErgoDox%20Acrylic%20Case/ErgoDOX%20Acrylic%20Case%20-%20Custom/Left/ErgoDOX%20Case%20Layer%205%20Bottom%20-%20Left.dxf)).

---

## 1. Left Half Coordinates

### A. Relative from Pattern Center `(0, 0)`
*Use this for center-aligned sketches in CAD/3D modeling software (Fusion 360, SolidWorks, FreeCAD).*

| Hole | Relative X (mm) | Relative Y (mm) | Description |
| :--- | :--- | :--- | :--- |
| **Top-Left (TL)** | **`-19.04`** | **`+19.78`** | Upper Row, Column 2/3 gap |
| **Top-Right (TR)** | **`+19.04`** | **`+18.31`** | Upper Row, Column 4/5 gap |
| **Bottom-Left (BL)** | **`-19.05`** | **`-18.29`** | Lower Row, Column 2/3 gap |
| **Bottom-Right (BR)** | **`+19.05`** | **`-19.79`** | Lower Row, Column 4/5 gap |

* **Pattern Center (in DXF Coordinate Space)**: `(X: 114.80 mm, Y: 95.69 mm)`

---

### B. Relative from Bottom-Left Corner `(0, 0)`
*Use this when dimensioning from a single datum point.*

| Hole | Relative X (mm) | Relative Y (mm) |
| :--- | :--- | :--- |
| **Bottom-Left (Origin)** | **`0.00`** | **`0.00`** |
| **Top-Left** | **`0.00`** *(+0.02)* | **`+38.07`** |
| **Bottom-Right** | **`+38.10`** | **`-1.50`** |
| **Top-Right** | **`+38.10`** | **`+36.60`** |

---

### C. Absolute DXF Coordinates (Left Half)

| Hole | DXF X (mm) | DXF Y (mm) | Feature in DXF |
| :--- | :--- | :--- | :--- |
| **Top-Left (TL)** | `95.77` | `115.47` | Original internal case Hole 9 |
| **Top-Right (TR)** | `133.85` | `114.00` | Added M3 hole |
| **Bottom-Left (BL)** | `95.75` | `77.40` | Added M3 hole |
| **Bottom-Right (BR)** | `133.85` | `75.90` | Added M3 hole |

---

## 2. Right Half Coordinates (Mirrored)

For the **Right keyboard half**, mirror across the vertical axis ($X \rightarrow -X$):

| Hole (Right Half) | Relative X from Center (mm) | Relative Y from Center (mm) |
| :--- | :--- | :--- |
| **Top-Left (Outer)** | **`-19.04`** | **`+18.31`** |
| **Top-Right (Inner)** | **`+19.04`** | **`+19.78`** |
| **Bottom-Left (Outer)** | **`-19.05`** | **`-19.79`** |
| **Bottom-Right (Inner)** | **`+19.05`** | **`-18.29`** |

---

## 3. Geometric Properties & Spacing

* **Horizontal Pitch ($\Delta X$)**: `38.10 mm` ($2 \times 19.05\text{ mm}$, standard 2 key-unit span).
* **Vertical Pitch ($\Delta Y$)**: `38.10 mm` ($2 \times 19.05\text{ mm}$, standard 2 key-unit span).
* **Column Stagger Offset**: `1.50 mm` vertical step down on the outer column (following the ergonomic finger stagger).
* **Diagonal Dimensions**:
  * Bottom-Left to Top-Right: `52.83 mm`
  * Bottom-Right to Top-Left: `54.92 mm`

---

## 4. Hardware & 3D Print Recommendations

1. **Hole Diameters on 3D Printed Holder**:
   - **Clearance Hole for M3 Screws**: `3.2 mm` to `3.4 mm`.
   - **For M3 Heat-Set Brass Inserts (e.g. Ruthex M3)**: `4.0 mm` to `4.2 mm` (check insert manufacturer specs).
2. **Screw Length & Material**:
   - Material: **2.0 mm Stainless Steel (нержавійка)**.
   - If screwing through Layer 5 into standoffs under Layer 3, account for the 2.0 mm plate thickness.
3. **Clearance**:
   - All 4 holes are positioned in the inter-column solid material channels with zero interference with switch pins, Kailh hotswap sockets, or PCB component pads.

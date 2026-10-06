import re
import plotly.graph_objects as go

with open("network_log.txt") as f:
    LOG_TEXT = f.read()

pattern = re.compile(
    r"\[None\]\s+(Client|Server):\s+([\d,]+\.\d+):\s+(.*)"
)

events = []
for line in LOG_TEXT.splitlines():
    m = pattern.search(line)
    if not m:
        continue

    side, timestamp, message = m.groups()
    t = float(timestamp.replace(",", ""))

    events.append({
        "side": side,
        "t": t,
        "message": message,
    })

if not events:
    raise ValueError("No matching Unreal log lines found.")

t0 = min(e["t"] for e in events)

for e in events:
    e["relative"] = e["t"] - t0


# ---------------------------------------------------------
# Event classification
# ---------------------------------------------------------

def category(message):
    if "Activate" in message:
        return "Activate"
    if "Raising heavy weapon" in message:
        return "Raise"
    if "No raise needed" in message:
        return "NoRaise"
    if "Shot" in message:
        return "Shot"
    if "End ability" in message:
        return "End"

    return "Other"


def short_label(message):
    cat = category(message)

    if cat == "Shot":
        # "Shot 3", "Shot 2", etc.
        m = re.search(r"Shot\s+(\d+)", message)
        return f"Shot {m.group(1)}" if m else "Shot"

    return {
        "Activate": "Activate",
        "Raise": "Raise",
        "NoRaise": "No raise",
        "End": "End",
        "Other": message[:20],
    }[cat]


# ---------------------------------------------------------
# Plot
# ---------------------------------------------------------

fig = go.Figure()

# Client / Server positions
Y = {
    "Client": 1,
    "Server": 0,
}

# Different vertical levels for labels.
# This prevents nearby labels from sitting directly on top
# of each other.
LABEL_LEVELS = {
    "Client": [1.20, 1.32, 1.44, 1.32, 1.20],
    "Server": [-0.20, -0.32, -0.44, -0.32, -0.20],
}


# ---------------------------------------------------------
# Main event points
# ---------------------------------------------------------

for side in ["Client", "Server"]:

    side_events = [
        e for e in events
        if e["side"] == side
    ]

    fig.add_trace(go.Scatter(
        x=[e["relative"] for e in side_events],
        y=[Y[side]] * len(side_events),

        mode="markers",

        name=side,

        marker=dict(
            size=10,
        ),

        customdata=[
            [
                e["message"],
                e["t"],
                category(e["message"]),
            ]
            for e in side_events
        ],

        hovertemplate=(
            "<b>%{customdata[2]}</b><br>"
            "%{customdata[0]}<br>"
            "Relative: %{x:.3f} s<br>"
            "Absolute: %{customdata[1]:.3f}"
            "<extra></extra>"
        ),
    ))


# ---------------------------------------------------------
# Labels + stems
# ---------------------------------------------------------

for side in ["Client", "Server"]:

    side_events = [
        e for e in events
        if e["side"] == side
    ]

    levels = LABEL_LEVELS[side]

    for i, e in enumerate(side_events):

        label_y = levels[i % len(levels)]

        # Stem from event to label
        fig.add_trace(go.Scatter(
            x=[e["relative"], e["relative"]],
            y=[Y[side], label_y],

            mode="lines",

            line=dict(
                width=1,
                dash="dot",
            ),

            showlegend=False,
            hoverinfo="skip",
        ))

        # Short label
        fig.add_trace(go.Scatter(
            x=[e["relative"]],
            y=[label_y],

            mode="text",

            text=[short_label(e["message"])],

            textposition="middle center",

            showlegend=False,
            hoverinfo="skip",
        ))


# ---------------------------------------------------------
# Connect corresponding Client / Server events
# ---------------------------------------------------------

client = [
    e for e in events
    if e["side"] == "Client"
]

server = [
    e for e in events
    if e["side"] == "Server"
]


for cat in ["Activate", "Raise", "NoRaise", "Shot", "End"]:

    c = [
        e for e in client
        if category(e["message"]) == cat
    ]

    s = [
        e for e in server
        if category(e["message"]) == cat
    ]

    for ce, se in zip(c, s):

        fig.add_trace(go.Scatter(
            x=[
                ce["relative"],
                se["relative"],
            ],

            y=[1, 0],

            mode="lines",

            line=dict(
                width=1,
                dash="dot",
            ),

            showlegend=False,
            hoverinfo="skip",
        ))


# ---------------------------------------------------------
# Axes
# ---------------------------------------------------------

fig.update_yaxes(
    tickvals=[1, 0],
    ticktext=["CLIENT", "SERVER"],

    range=[
        -0.55,
        1.55,
    ],

    fixedrange=True,
)

fig.update_xaxes(
    title="Seconds from first event",

    showgrid=True,

    rangeslider=dict(
        visible=True,
    ),
)


# ---------------------------------------------------------
# Layout
# ---------------------------------------------------------

fig.update_layout(
    title="Unreal weapon-up timeline",

    height=650,

    hovermode="closest",

    dragmode="zoom",

    margin=dict(
        l=80,
        r=40,
        t=60,
        b=80,
    ),
)

fig.show()
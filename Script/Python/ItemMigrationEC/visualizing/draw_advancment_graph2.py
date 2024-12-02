import math

import plotly.graph_objects as go
import networkx as nx


def generate_html(dependencies, positions, icon_dir, output_path="graph.html", canvas_size=(2000, 2000)):
    html_content = f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Icon Dependency Graph</title>
    <style>
        #graphCanvas {{
            border: 1px solid #ccc;
            width: {canvas_size[0]}px;
            height: {canvas_size[1]}px;
            background-color: #000000;
        }}
        .tooltip {{
            position: absolute;
            padding: 5px;
            background-color: #333;
            color: #fff;
            border-radius: 3px;
            font-size: 14px;
            display: none;
            pointer-events: none;
        }}
    </style>
</head>
<body>

<canvas id="graphCanvas" width="{canvas_size[0]}" height="{canvas_size[1]}"></canvas>
<div id="tooltip" class="tooltip"></div>

<script>
    const dependencies = {dependencies};
    const positions = {positions};
    const iconDir = "{icon_dir}";

    const canvas = document.getElementById("graphCanvas");
    const ctx = canvas.getContext("2d");
    const tooltip = document.getElementById("tooltip");

    const icons = {{}};
    const defaultImage = new Image();
    defaultImage.src = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAOEAAADhCAMAAAAJbSJIAAAAOVBMVEX///+Hh4d4eHhqampcXFxRUVGDg4NaWlpUVFQqKiqMjIwICAgwMDA2NjY/Pz9ERERzc3NmZmZISEhIlpe1AAABgUlEQVR4nO3dOW4DMRAF0eYsmsWLRrr/Ya3AiZRYMNBo1LAeL/ArJECA8bF9fn1fr/M8r9PD+OsYnrR/uwx/OsY33KY3LPOrdYslzm2PqXpCstZB4a16QrIWY/WEZBbyWchnIZ+FfBby9VB4VE9I1mKonpDMQj4L+Szks5DPQj4L+Szks5DPQj4L+Szks5DPQj4L+Szks5CvPc65WchnIZ+FfBbyWchnIZ+FfBbyWchnIZ+FfBbyWchnIZ+FfBbyWchnIZ+FfC0u1ROS9fBiyEI6C/ks5LOQz0I+C/ks5LOQz0I+C/ks5LOQz0I+C/ks5LOQz0I+C/ks5LOQr4d/Ziyks5DPQj4L+Szks5DPQj4L+Szks5DPQj4L+Szks5DPQj4L+Szks5CvxVQ9IZmFfBbyWchnIZ+FfBbyWchnIZ+FfBbyWchnIZ+FfBbyWchnIZ+FfD0ULtUTkrVYqyckazFXT0hmIZ+FfBbyWchnIZ+FfBby9VB49hvwHtvezmy//wAfTAdL4oo7JgAAAABJRU5ErkJggg==";

    // Load all icons, then draw the graph
    async function loadIconsAndDraw() {{
        for (const name in positions) {{
            const img = new Image();
            img.src = `${{iconDir}}/${{name}}.png`;
            icons[name] = img;

            await new Promise(resolve => {{
                img.onload = resolve;
                img.onerror = () => {{
                    icons[name] = defaultImage;
                    resolve();
                }};
            }});
        }}
        drawGraph();
    }}

    function drawGraph() {{
        ctx.clearRect(0, 0, canvas.width, canvas.height);

        // Draw arrows for dependencies
        ctx.strokeStyle = "white";
        ctx.lineWidth = 2;
        for (const src in dependencies) {{
            const [x1, y1] = positions[src];
            for (const dest of dependencies[src]) {{
                const [x2, y2] = positions[dest];
                drawArrow(x1, y1, x2, y2);
            }}
        }}

        // Draw icons
        for (const name in positions) {{
            const [x, y] = positions[name];
            const img = icons[name];
            if (img && img.complete) {{
                ctx.drawImage(img, x - img.width / 2, y - img.height / 2);
            }}
        }}
    }}

    function drawArrow(x1, y1, x2, y2) {{
        const headlen = 10;
        const angle = Math.atan2(y2 - y1, x2 - x1);
        ctx.beginPath();
        ctx.moveTo(x1, y1);
        ctx.lineTo(x2, y2);
        ctx.lineTo(x2 - headlen * Math.cos(angle - Math.PI / 6), y2 - headlen * Math.sin(angle - Math.PI / 6));
        ctx.moveTo(x2, y2);
        ctx.lineTo(x2 - headlen * Math.cos(angle + Math.PI / 6), y2 - headlen * Math.sin(angle + Math.PI / 6));
        ctx.stroke();
    }}

    canvas.addEventListener("mousemove", event => {{
        const rect = canvas.getBoundingClientRect();
        const mouseX = event.clientX - rect.left;
        const mouseY = event.clientY - rect.top;
        let found = false;

        for (const name in positions) {{
            const [x, y] = positions[name];
            const img = icons[name];
            const halfWidth = img.width / 2;
            const halfHeight = img.height / 2;

            if (mouseX >= x - halfWidth && mouseX <= x + halfWidth &&
                mouseY >= y - halfHeight && mouseY <= y + halfHeight) {{
                tooltip.style.left = `${{event.pageX + 10}}px`;
                tooltip.style.top = `${{event.pageY + 10}}px`;
                tooltip.style.display = "block";
                tooltip.textContent = name;
                found = true;
                break;
            }}
        }}
        if (!found) {{
            tooltip.style.display = "none";
        }}
    }});

    canvas.addEventListener("click", event => {{
        const rect = canvas.getBoundingClientRect();
        const mouseX = event.clientX - rect.left;
        const mouseY = event.clientY - rect.top;

        for (const name in positions) {{
            const [x, y] = positions[name];
            const img = icons[name];
            const halfWidth = img.width / 2;
            const halfHeight = img.height / 2;

            if (mouseX >= x - halfWidth && mouseX <= x + halfWidth &&
                mouseY >= y - halfHeight && mouseY <= y + halfHeight) {{
                navigator.clipboard.writeText(name);
                break;
            }}
        }}
    }});

    loadIconsAndDraw();
</script>

</body>
</html>
    """

    with open(output_path, "w") as file:
        file.write(html_content)
    print(f"HTML file generated at {output_path}")

if __name__ == "__main__":
    import pandas as pd

    graph = {}
    text_dict = {}
    pos_dict = {}
    df = pd.read_csv("../data/ec/advancements/csm.csv")
    df = df.fillna("")
    min_x, min_y = math.inf, math.inf
    max_x, max_y = -math.inf, -math.inf

    for i, row in df.iterrows():
        row = row.to_dict()
        my_node = row.get("Progression")
        dependency_node = row.get("Dependency")
        ui_node = row.get("ui_position")

        if dependency_node:
            graph[dependency_node] = graph.get(dependency_node, []) + [my_node]
        else:
            graph[my_node] = []

        text_dict[
            my_node] = f"{my_node} ({row.get('Rank', 1)})<br>Name: {row.get('Name')}<br>Item: {row.get('ItemReward')}"
        x = float(ui_node.split(",")[0])
        y = float(ui_node.split(",")[1])
        min_x, min_y = min(min_x, x), min(min_y, y)
        max_x, max_y = max(max_x, x), max(max_y, y)
        pos_dict[my_node] = [x, y]

    canvas_size = (max_x - min_x + 200, max_y - min_y + 200)
    pos_dict = {k: [v[0] - min_x + 100, v[1] - min_y + 100] for k, v in pos_dict.items()}
    icon_dir = "C:/Users/JediKnight/Downloads/ChaosAdvancementsIcons/ChaosAdvancements/"
    generate_html(graph, pos_dict, icon_dir, "../data/ecr/advancements/csm_graph2.html", canvas_size)

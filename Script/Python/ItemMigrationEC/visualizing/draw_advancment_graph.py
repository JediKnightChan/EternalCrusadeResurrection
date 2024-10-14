import plotly.graph_objects as go
import networkx as nx


def iterative_hierarchical_pos(G, root=None, width=1., vert_gap=0.2, xcenter=0.5):
    pos = {}
    queue = [(root, xcenter, 0)]  # Start with the root node at the top center

    while queue:
        node, x, y = queue.pop(0)
        pos[node] = (x, -y)  # Invert y to make the graph top-down

        neighbors = list(G.neighbors(node))
        if len(neighbors) > 0:
            dx = width / len(neighbors)
            next_x = x - width / 2 - dx / 2
            for neighbor in neighbors:
                next_x += dx
                queue.append((neighbor, next_x, y + vert_gap))

    return pos


def draw_graph(graph_dict, text_dict, output_fp):
    G = nx.DiGraph()

    # Add edges to the graph
    for node, neighbors in graph_dict.items():
        for neighbor in neighbors:
            G.add_edge(node, neighbor)

    pos = {}
    x_offset = 0  # Start x offset for the first component
    component_spacing = 1  # Increased space between components

    # Place each weakly connected component separately
    for component in nx.weakly_connected_components(G):
        component_subgraph = G.subgraph(component)

        # Identify root node or start with an arbitrary node if cyclic
        try:
            root = [n for n, d in component_subgraph.in_degree() if d == 0][0]
        except IndexError:
            root = list(component_subgraph.nodes())[0]  # Pick an arbitrary node

        component_pos = iterative_hierarchical_pos(component_subgraph, root=root, width=1., vert_gap=0.4)

        # Apply horizontal offset to avoid overlap between components
        component_pos = {node: (x + x_offset, y) for node, (x, y) in component_pos.items()}
        pos.update(component_pos)

        # Update the horizontal offset for the next component
        x_offset += component_spacing

    # Create edge trace
    edge_trace = go.Scatter(
        x=(),
        y=(),
        line=dict(width=0.5, color='#888'),
        hoverinfo='none',
        mode='lines'
    )

    for edge in G.edges():
        x0, y0 = pos[edge[0]]
        x1, y1 = pos[edge[1]]
        edge_trace['x'] += (x0, x1, None)
        edge_trace['y'] += (y0, y1, None)

    # Create node trace
    node_trace = go.Scatter(
        x=(),
        y=(),
        text=(),  # For abbreviated names
        hovertext=(),  # For full names
        mode='markers+text',
        textposition="top center",
        hoverinfo='text',
        marker=dict(
            showscale=True,
            colorscale='YlGnBu',
            size=20,
            colorbar=dict(
                thickness=15,
                title='Node Connections',
                xanchor='left',
                titleside='right'
            ),
            line_width=2
        ),
        textfont=dict(
            size=10  # Decreased font size for node labels
        )
    )

    for node in G.nodes():
        x, y = pos[node]
        # Get the last part of the node name for display
        short_name = node.split("_")[-1]
        node_trace['x'] += (x,)
        node_trace['y'] += (y,)
        node_trace['text'] += (short_name,)

        node_trace['hovertext'] += (text_dict.get(node),)

    # Add the number of connections as the color of the nodes
    node_adjacencies = []
    for node, adjacencies in enumerate(G.adjacency()):
        node_adjacencies.append(len(adjacencies[1]))
    node_trace.marker.color = node_adjacencies

    # Create the figure
    fig = go.Figure(data=[edge_trace, node_trace],
                    layout=go.Layout(
                        title='<br>Network Graph',
                        titlefont_size=16,
                        showlegend=False,
                        hovermode='closest',
                        margin=dict(b=0, l=0, r=0, t=50),
                        annotations=[dict(
                            text="",
                            showarrow=False,
                            xref="paper", yref="paper"
                        )],
                        dragmode='pan',
                        xaxis=dict(showgrid=False, zeroline=False),
                        yaxis=dict(showgrid=False, zeroline=False))
                    )

    # Save the figure to an HTML file
    config = dict({'scrollZoom': True})
    fig.write_html(output_fp, config=config)
    print(f"Graph saved as {output_fp}")


if __name__ == "__main__":
    import pandas as pd

    graph = {}
    text_dict = {}
    df = pd.read_csv("./data/ec/advancements/lsm.csv")
    df = df.fillna("")

    for i, row in df.iterrows():
        row = row.to_dict()
        my_node = row.get("Progression")
        dependency_node = row.get("Dependency")

        if dependency_node:
            graph[dependency_node] = graph.get(dependency_node, []) + [my_node]
        else:
            graph[my_node] = []

        text_dict[my_node] = f"{my_node} ({row.get('Rank', 1)})<br>Name: {row.get('Name')}<br>Item: {row.get('ItemReward')}"

    # Draw the graph and save it to an HTML file
    draw_graph(graph, text_dict, "./data/ecr/advancements/lsm_graph.html")

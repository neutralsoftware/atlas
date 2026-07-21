import { Component } from "atlas";
import { Debug } from "atlas/log";
import { Button } from "graphite";

export class UIStatus extends Component {
    init() {
        const button = this.getParent() as Button;
        button.setOnClick(() => {
            button.setLabel("Graphite works");
            Debug.print("Graphite UI button clicked");
        });
    }

    update(_deltaTime: number) {}
}

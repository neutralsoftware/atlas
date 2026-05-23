import { ContextMenuItem } from "@shared/types/ipc";

declare global {
    interface Window {
        contextMenu: {
            show(items: ContextMenuItem[]): void

            onClick: (
                callback: (action: string) => void
            ) => void
        }
    }
}
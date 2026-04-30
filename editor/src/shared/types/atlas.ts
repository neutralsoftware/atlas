export type Project = {
    name: string;
    starred: boolean;
    path: string;
    modified: Date;
    id: string;
};

export type GameObject = {
    id?: number;
    viewportId: number;
    name: string;
    type: string;
    children?: GameObject[];
};

export type Scene = {
    name: string;
    objects: GameObject[];
    selectedId?: number;
};

int main(){
    void ** c = malloc(sizeof(int));
    void * d = (void *) c;

    printf("%p\n", d);
    free(c);
    return 0;
}
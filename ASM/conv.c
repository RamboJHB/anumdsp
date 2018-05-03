#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TMP_STR_SIZE 256
#define MEM_MAX 0x81000000
#define MEM_OK  0x80002001


extern som_prod_asm();
//Variable qui permet de pointer vers l'emplacement m�moire du DSP o� est stock� l'image brut
unsigned char* tab_image_source;
//Variable qui permet de pointer vers l'emplacement m�moire du DSP o� est stock� l'image trait�e(apr�s filtrage)
unsigned char* tab_image_final;
//Largeur de l'image
unsigned int width; 
//Hauteur de l'image
unsigned int height; 
//Variable qui permet de r�cup�rer la valeur max du ou des pixels contenu dans l'image
unsigned char color_max;
//Variable de type �num�r�e qui permet d'identifier si une image est de type Binaire ou ASCII
enum format {BIN, ASCII} pgm_form;
//Prototype de la fonction qui permet de lire une image et de r�cup�rer son contenu
void open_file (char* fichier_source);
//Prototype de la fonction qui permet d'enregistrer dans un fichier image (.pgm) l'image filtr�e situ� dans un emplacement m�moire du DSP point� par le pointeur "tab_image_final"
void write_file (unsigned char *emplacement_memoire_img_filtre, unsigned m);



//Fonction qui permet de lire une image et de r�cup�rer son contenu
void open_file (char* fichier_source) {
	
	char tmp_str[TMP_STR_SIZE];
	char caractere;
	char temp[4];
	unsigned long int val = 0;
	long position = 0;
	int on_detect=0;
	int i = 0;
	int cmpt=0;
	int on =0;
	int cmpt_espace=0;
	int caractereLu;
	FILE* source = NULL;
	unsigned char test;
    
	width = 0;
	height = 0;
	color_max = 0;
	tab_image_source = NULL;
	tab_image_final = NULL;


	//Ouverture du fichier contenant l'image brut
	source = fopen (fichier_source, "r");
	//On v�rifie que le fichier est bien existant
	//Si ce n'est pas le cas, on l�ve une exception et on affiche un message d'erreur
	if (!source) 
	{
		//Affichage du message d'erreur
		fprintf (stderr, "Fichier inexistant\n"); 
		return;
	}
	do 
	{ 
		fgets(tmp_str, TMP_STR_SIZE, source); 
	} 
	while (tmp_str[0]=='#');
	//On identifie le type d'image qui a �t� ouvert
	//On verifie le type est bien connu , type "P" pour Pgm.
	if (tmp_str[0]=='P') {
		//On regarde si l'image est de type P2
		//Si c'est le cas alors l'image est de type de ASCII
		if (tmp_str[1]=='2')
		{ 
			pgm_form=ASCII; 
		}
		//Sinon si l'image de type P5 alors l'image est de type binaire
		else if (tmp_str[1]=='5')
		{ 
			pgm_form=BIN;
		}
		//Sinon si l'image n'est ni du type P5 ou P2 on affiche un message d'erreur indiquant que le format n'est pas connu
		else 
		{ 
			//Affichage du message d'erreur
			fprintf (stderr, "Erreur de format P2 P5\n");
			//Fermeture du fichier contenant l'image brut
			fclose(source); 
			return; 
		}
	}
	//Si le format n'est pas de Type Px(P2 ou P5) , on affiche un message d'erreur
	else 
	{
		//Affichage du message d'erreur
		fprintf (stderr, "Erreur de format P2 P5\n"); 
		//Fermeture du fichier contenant l'image brut
		fclose(source); 
		return; 
	}
	/* le fichier est lisible correctement */
	do 
	{ 
		fgets(tmp_str, TMP_STR_SIZE, source);
	}while (tmp_str[0]=='#');
	//On recup�re le nombre de ligne et de colonne de l'image
	sscanf(tmp_str, "%d %d", &(width), &(height)); 
	//Si l'image fait 4*4 on affiche un message d'erreur car elle est trop petite pour �tre tait�e
	if ((width<4)||(height<4)) 
	{
		//Affichage du message d'erreur
		printf("Impossible image trop petite \n");
		//Fermeture du fichier contenant l'image brut
		fclose(source);
		return;
	}
	fgets(tmp_str, TMP_STR_SIZE, source);
	//On recup�re la valeur maximale du ou des pixels de l'image brut
	sscanf(tmp_str, "%d", &(color_max));
	//La valeur d'un pixel en hexadecimal est compris entre 0 et 255
	//Si cette valeur est sup�rieur � 255 alors on affiche un message d'erreur
	if (color_max>255) 
	{
		//Affichage du message d'erreur
		printf("Impossible color_max > 255\n");
		//Fermeture du fichier contenant l'image brut
		fclose(source);
		return;
	}
	//Sinon si cette valeur est inf�rieur � 0 alors on affiche un message d'erreur
	else if(color_max<0) 
	{
		//Affichage du message d'erreur
		printf("Impossible color_max < 0\n");
		//Fermeture du fichier contenant l'image brut
		fclose(source);
		return;
	}
	//On positionnne le pointeur tab_image_source vers l'emplacement m�moire du DSP qui contiendra l'image brut
	tab_image_source = (unsigned char*) MEM_OK;
	//On positionnne le pointeur tab_image_final vers l'emplacement m�moire du DSP qui contiendra l'image filtr�
	tab_image_final  = (unsigned char*) (MEM_OK + (width*height));
	//Si la taille de l'image brut est sup�rieure � taille m�moire allou� pour le stockage de celle-ci dans le DSP
	//alors on affiche un message d'erreur
	if ((((width*height)+1)*2)> MEM_MAX) 
	{
		//Affichage du message d'erreur
		printf("Impossible memoire insuffisante pour charger l'image d'origine et celle modifie\n");
		//Fermeture du fichier contenant l'image brut
		fclose(source);
		return; 
	}
	val=0;
	caractereLu=fgetc(source);
    position=ftell(source);
    caractere=(char)caractereLu;
	//Algorithme qui permet de recuperer caract�re par caract�re la valeur d'un pixel
	//On stocke dans un tableau de caract�re les caract�res lues
	//Si le caract�re d�tect� est un espace ou un retour chariot
	//alors on ferme la chaine de caract�re avec le caract�re '\0'
	//Pour ensuite la stocker dans la m�moire du DSP
    while(caractereLu!=EOF)
    {
        if(caractere!=' ')
        {
            on = 0;
            if(caractere !='\n')
            {
                on_detect=0;
                temp[i]=caractere;
                i=i+1;
            }
            else if(caractere == '\n')
            {
                cmpt_espace=0;
                on_detect=0;
                temp[i]='\0';
                test = (unsigned char) atoi(temp);
                tab_image_source[val]=test;
                i=0;
                strcpy(temp," ");
                val++;
            }
        }
        else if(caractere == ' ')
        {
            if(on_detect==0)
            {
                cmpt_espace=0;
            }
            on_detect=1;
            cmpt_espace = cmpt_espace + 1;
            caractereLu = fgetc(source);
            caractere = (char)caractereLu;
            if(caractere!='\n')
            {
                if(cmpt_espace==1 && on_detect==1)
                {
                    temp[i]='\0';
                    test = (unsigned char) atoi(temp);
                    tab_image_source[val]=test;
                    val++;
                    i=0;
                    strcpy(temp," ");
                }
                else if(cmpt_espace==2 && on_detect==1)
                {
                    cmpt_espace=0;
                }
            }
            on=1;
        }
        if(on==0)
        {
            caractereLu=fgetc(source);
            caractere=(char)caractereLu;
        }
    }
	fclose(source);
	printf ("image �crite en m�moire \n");
	printf("\n");
}



//Fonction qui permet d'enregistrer dans un fichier image (.pgm) 
//l'image filtr�e situ� dans un emplacement m�moire du DSP point� par le pointeur "tab_image_final"
void write_file (unsigned char *emplacement_memoire, unsigned m)
{

	unsigned long int val = 0;
	FILE* nouveau = NULL;
	int cmpt=0;
	int origine = width*height;
	int indice_memoire_image_filtre = 0;
	
	//On creee le fichier qui contiendra l'image filtr�
	nouveau=fopen("Output.pgm","w");
	//Si le fichier n'a pa pu �tre creee on affiche un message d'erreur
	if (nouveau == NULL) 
	{
		//Affichage du message d'erreur
		printf("Impossible d'ouvrir le fichier nouveau Output.pgm \n");
		return;
	}
	/*******************************************************************************************************************************/
	//											Dans cette partie on creee l'ent�te
	/*******************************************************************************************************************************/
	//Si le fichier brut est de type Binaire, alors dans le fichier contenant l'image filtr� on met en ent�te P5
	if (pgm_form==BIN) 
	{
		fputs("P5\n", nouveau); 
	} 
	//Sinon on met en ent�te P2
	else 
	{
		fputs("P2\n", nouveau); 
	}
	//On place � la suite du type de l'image , sa longueur et sa largeur
	fprintf(nouveau, "%i %i\n", width, height);
	//On place � la suite de la longueur et de la largeur de l'image, la valeur maximale du ou des pixels de l'image filtr� 
	fprintf(nouveau, "%i\n", m);
	/*******************************************************************************************************************************/
	//										Fin de cr�ation de l'ent�te de l'image filtr�e
	/*******************************************************************************************************************************/
	
	//On place � la suite de l'ent�te du fichier filtr�, le contenu des diff�rents pixels sur lesquels le filtre a �t� appliqu�
	for (val = 0; val < (width*height)-1; val++) 
	{
		//Si le format de l'image brut est de type ASCII
		//alors l'ecriture dans le fichier ce fera sous format hexadecimal
		if (pgm_form == ASCII) 
		{
			cmpt = 0;
			fprintf(nouveau, "%i\n", *(emplacement_memoire+val));
		}
		//Sinon si le format de l'image brut est de type Binaire 
		//alors l'ecriture dans le fichier ce fera sous format caract�re
		else 
		{
			fputc(	tab_image_source[val], nouveau);
		}
	}
	fclose(nouveau);
	printf("fichier ecrit \n");
}

short som_prod_c(unsigned char * pix_buf,short* coeff_buf)
{ 
	short count,sum;
	sum=0;
	for(count=0 ; count < 9 ; count++)
		sum += coeff_buf[count]*pix_buf[count];
		
	return(sum);	       
}


//Programme principal
void main()
{   
	short val;
	unsigned char pixel_max = 0;
	short coeff_buf[]={-1,-1,-1,-1,8,-1,-1,-1,-1};
	unsigned char pix_buf[9]={0,0,0,0,0,0,0,0,0};
	short k = 0;
	printf("loading file...\n");
	//Appel de la fonction qui permet de lire l'image "lena.ascii.pgm" et de r�cup�rer son contenu
	open_file ("pepper.pgm");
	printf("calculating...\n");
	//unsigned char * image_source =0x80002001;
    for(k=0;k<width*height;k++){
	//printf("k = %d \n", k); 

		if(k==0){
			pix_buf[0]=0;
			pix_buf[1]=0;
			pix_buf[2]=0;
			pix_buf[3]=0;
			pix_buf[4]=tab_image_source[k];
			pix_buf[5]=tab_image_source[k+1];
			pix_buf[6]=0; 
			pix_buf[7]=tab_image_source[k+width];
			pix_buf[8]=tab_image_source[k+width+1];
		}
		else if(k==(width-1)){
		      pix_buf[0]=0;
		      pix_buf[1]=0;
		      pix_buf[2]=0;
		      pix_buf[3]=tab_image_source[k-1];
		      pix_buf[4]=tab_image_source[k];
		      pix_buf[5]=0;
		      pix_buf[6]=tab_image_source[k+width-1]; 
		      pix_buf[7]=tab_image_source[k+width];
		      pix_buf[8]=0;
		}else if(k==(height-1)*width){
      
		      pix_buf[0]=0;
		      pix_buf[1]=tab_image_source[k-width];
		      pix_buf[2]=tab_image_source[k-width+1];
		      pix_buf[3]=0;
		      pix_buf[4]=tab_image_source[k];
		      pix_buf[5]=tab_image_source[k+1];
		      pix_buf[6]=0; 
		      pix_buf[7]=0;
		      pix_buf[8]=0;
		}else if(k==(height*width-1)){
      
		      pix_buf[0]=tab_image_source[k-width-1];
		      pix_buf[1]=tab_image_source[k-width];
		      pix_buf[2]=0;
		      pix_buf[3]=tab_image_source[k-1];
		      pix_buf[4]=tab_image_source[k];
		      pix_buf[5]=0;
		      pix_buf[6]=0; 
		      pix_buf[7]=0;
		      pix_buf[8]=0;
		}else if(k<width-1){
		      pix_buf[0]=0;
		      pix_buf[1]=0;
		      pix_buf[2]=0;
		      pix_buf[3]=tab_image_source[k-1];
		      pix_buf[4]=tab_image_source[k];
		      pix_buf[5]=tab_image_source[k+1];
		      pix_buf[6]=tab_image_source[k+width-1]; 
		      pix_buf[7]=tab_image_source[k+width];
		      pix_buf[8]=tab_image_source[k+width+1];
		}else if((height-1)*width<k && k<(height*width-1)){
		      pix_buf[0]=tab_image_source[k-width-1];
		      pix_buf[1]=tab_image_source[k-width];
		      pix_buf[2]=tab_image_source[k-width+1];
		      pix_buf[3]=tab_image_source[k-1];
		      pix_buf[4]=tab_image_source[k];
		      pix_buf[5]=tab_image_source[k+1];
		      pix_buf[6]=0; 
		      pix_buf[7]=0;
		      pix_buf[8]=0;	 
		}else if((k%width==0)&&(k!=0)&&(k!=(height-1)*width)){
      
		      pix_buf[0]=tab_image_source[k-width-1];
		      pix_buf[1]=0;
		      pix_buf[2]=tab_image_source[k-width+1];
		      pix_buf[3]=0;
		      pix_buf[4]=tab_image_source[k];
		      pix_buf[5]=tab_image_source[k+1];
		      pix_buf[6]=0; 
		      pix_buf[7]=tab_image_source[k+width];
		      pix_buf[8]=tab_image_source[k+width+1];
			  
		}

		else if(((k+1)%width==0)&&(k!=width-1)&&(k!=height*width-1)){ 
		      pix_buf[0]=tab_image_source[k-width-1];
		      pix_buf[1]=tab_image_source[k-width];
		      pix_buf[2]=0;
		      pix_buf[3]=tab_image_source[k-1];
		      pix_buf[4]=tab_image_source[k];
		      pix_buf[5]=0;
		      pix_buf[6]=tab_image_source[k+width-1]; 
		      pix_buf[7]=tab_image_source[k+width];
		      pix_buf[8]=0;  
		}else{  
	          pix_buf[0]=tab_image_source[k-width-1];
			  pix_buf[1]=tab_image_source[k-width];
			  pix_buf[2]=tab_image_source[k-width+1];
			  pix_buf[3]=tab_image_source[k-1];
		      pix_buf[4]=tab_image_source[k];
			  pix_buf[5]=tab_image_source[k+1];
			  pix_buf[6]=tab_image_source[k+width-1]; 
			  pix_buf[7]=tab_image_source[k+width];
			  pix_buf[8]=tab_image_source[k+width+1];	  
		}   	          

        //val = som_prod_asm(pix_buf, coeff_buf);
		val = som_prod_c(pix_buf, coeff_buf);
		// check max pixel
		if (val< 0){val = 0;}
		if (val > 255){val = 255;}
		//tab_image_source[k]=val;
        tab_image_final[k]=(unsigned char) val;
		

		if (pixel_max<tab_image_final[k])
		{
			pixel_max = tab_image_final[k];
		}
		 
		//printf(" k = %d\n",width*height-1-k);
	}
        write_file(tab_image_final,pixel_max);
		printf("finished\n");



}

